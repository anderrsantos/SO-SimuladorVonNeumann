#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp"
#include "multicore/MultiCore.hpp"
#include "multicore/Scheduler.hpp"
#include "cpu/PCB.hpp"
#include "cpu/pcb_loader.hpp"
#include "metrics/Metrics.hpp"

using namespace std;
namespace fs = std::filesystem;

// -------------------------------------------------------------
// Resolver arquivos de processo
// -------------------------------------------------------------
static vector<string> resolve_process_files(int argc, char** argv) {
    vector<string> files;

    auto exists = [&](fs::path p) {
        return fs::exists(p) && fs::is_regular_file(p);
    };

    // 1) Se argumentos foram dados
    for (int i = 1; i < argc; i++) {
        fs::path p = argv[i];

        if (exists(p))                             files.push_back(p.string());
        else if (exists(fs::path("processes")/p))  files.push_back((fs::path("processes")/p).string());
        else if (exists(fs::path("..")/"processes"/p)) 
                                                   files.push_back((fs::path("..")/"processes"/p).string());
        else if (p.extension() == ".json")         files.push_back(p.string());
    }

    // 2) Procurar ./processes
    if (files.empty() && fs::exists("processes")) {
        for (auto &e : fs::directory_iterator("processes"))
            if (e.path().extension() == ".json")
                files.push_back(e.path().string());
    }

    // 3) Procurar ../processes
    if (files.empty() && fs::exists("../processes")) {
        for (auto &e : fs::directory_iterator("../processes"))
            if (e.path().extension() == ".json")
                files.push_back(e.path().string());
    }

    return files;
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char** argv) {

    ios::sync_with_stdio(false);

    // ------------------------ CONFIGURAÇÃO ------------------------
    const size_t RAM_SIZE       = 4096;
    const size_t SEC_SIZE       = 8192;
    const size_t CACHE_CAP      = 64;
    const uint32_t PART_SIZE    = 512;
    const size_t NCORES         = 4;

    SchedPolicy policy = SchedPolicy::FCFS;

    // detectar se primeiro argumento é política
    if (argc >= 2) {
        string pol = argv[1];
        if      (pol == "fcfs")     policy = SchedPolicy::FCFS;
        else if (pol == "rr")       policy = SchedPolicy::RR;
        else if (pol == "priority") policy = SchedPolicy::PRIORITY;
        else if (pol == "sjn")      policy = SchedPolicy::SJN;
    }

    // ------------------------ ARQUIVOS DE PROCESSO ------------------------
    vector<string> files = resolve_process_files(argc, argv);

    if (files.empty()) {
        cerr << "[main] Nenhum arquivo .json encontrado em ./processes ou ../processes.\n";
        return 1;
    }

    cout << "[main] Arquivos carregados:\n";
    for (auto &f : files) cout << "   " << f << "\n";

    // ------------------------ COMPONENTES ------------------------
    MemoryManager memory(RAM_SIZE, SEC_SIZE, CACHE_CAP);
    memory.createPartitions(PART_SIZE);

    IOManager ioManager;
    Scheduler scheduler(policy);
    MultiCore multicore(NCORES, &memory, &ioManager, nullptr);

    uint64_t tick = 0;

    // ------------------------ CARREGAR PCBs ------------------------
    vector<unique_ptr<PCB>> allPCBs;
    vector<PCB*> pcbPtrs;

    for (auto &file : files) {
        auto up = make_unique<PCB>();

        if (!load_pcb_from_json(file, *up)) {
            cerr << "[main] Erro ao carregar " << file << "\n";
            continue;
        }

        up->arrival_time = tick;

        pcbPtrs.push_back(up.get());
        allPCBs.push_back(std::move(up));
    }

    if (pcbPtrs.empty()) {
        cerr << "[main] Nenhum PCB válido.\n";
        return 1;
    }

    // ------------------------ ALOCAR EM MEMÓRIA ------------------------
    vector<PCB*> pending;

    for (PCB* p : pcbPtrs) {
        uint32_t req = p->data_bytes + p->code_bytes;
        if (req == 0) req = 1;

        Partition* part = memory.allocateFixedPartition(*p, req);

        if (!part) {
            pending.push_back(p);
            continue;
        }

        // DATA
        for (uint32_t i = 0; i < p->data_bytes; i++)
            memory.writeLogical(i, p->dataSegment[i], *p);

        // CODE
        for (uint32_t i = 0; i < p->code_bytes; i++)
            memory.writeLogical(p->data_bytes + i, p->codeSegment[i], *p);

        p->initial_pc = p->data_bytes;
        scheduler.add(p);
    }

    // fetchNext wrapper
    auto fetchNext = [&](Scheduler &sched, uint64_t T) {
        PCB* p = sched.fetchNext();
        if (p && p->start_time == 0) {
            p->start_time = T;
            p->response_time = T - p->arrival_time;
        }
        return p;
    };

    // ------------------------ LOOP PRINCIPAL ------------------------
    while (!scheduler.empty()
           || multicore.hasActiveCores()
           || !pending.empty()
           || ioManager.pendingCount() > 0) {

        // tentar alocar pendentes
        if (!pending.empty()) {
            vector<PCB*> remain;

            for (PCB* p : pending) {
                uint32_t req = p->data_bytes + p->code_bytes;
                if (req == 0) req = 1;

                Partition* part = memory.allocateFixedPartition(*p, req);

                if (!part) {
                    remain.push_back(p);
                    continue;
                }

                for (uint32_t i = 0; i < p->data_bytes; i++)
                    memory.writeLogical(i, p->dataSegment[i], *p);

                for (uint32_t i = 0; i < p->code_bytes; i++)
                    memory.writeLogical(p->data_bytes + i, p->codeSegment[i], *p);

                p->initial_pc = p->data_bytes;
                scheduler.add(p);
            }

            pending.swap(remain);
        }

        // enviar processos para núcleos
        multicore.assignReadyProcesses([&]() {
            return fetchNext(scheduler, tick);
        });

        // avançar núcleos
        auto events = multicore.stepAll();

        // processar eventos
        for (auto &ev : events) {
            PCB* p = ev.pcb;

            if (ev.type == CoreEvent::FINISHED) {
                p->finish_time = tick;

                uint64_t ta = p->finish_time - p->arrival_time;
                uint64_t service = p->pipeline_cycles.load();

                p->wait_time = (ta > service ? ta - service : 0);

                memory.freePartition(p->pid);
            }
            else if (ev.type == CoreEvent::BLOCKED) {
                ioManager.registerProcessWaitingForIO(ev.pcb, std::move(ev.ioRequests), 120);
            }
            else if (ev.type == CoreEvent::PREEMPTED) {
                scheduler.add(ev.pcb);
            }
        }

        ioManager.step();
        tick++;
    }

    // ------------------------ FLUSH CACHE ------------------------
    for (auto &p : memory.L1_cache->dirtyData())
        memory.writeToFile(p.first, p.second);

    // ------------------------ MÉTRICAS ------------------------
    auto reports = Metrics::collect(allPCBs);

    Metrics::printConsole(reports);
    //Metrics::saveCSV(reports, "output/metrics.csv");
    //Metrics::saveJSON(reports, "output/metrics.json");

    cout << "\n[main] Simulação finalizada. Resultados em /output\n";

    return 0;
}
