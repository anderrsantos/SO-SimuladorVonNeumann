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
#include "metrics/MetricsExtended.hpp"
#include "metrics/TemporalMetrics.hpp"
#include <filesystem>

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
    cin.tie(nullptr);

    // ------------------------ CONFIGURAÇÃO ------------------------
    const size_t RAM_SIZE       = 4096;   // em WORDS
    const size_t SEC_SIZE       = 8192;   // em WORDS
    const size_t CACHE_CAP      = 64;
    const uint32_t PART_SIZE    = 512;
    size_t NCORES                = 4;  // Padrão: 4 cores

    SchedPolicy policy = SchedPolicy::FCFS;

    // Detectar argumentos: política e número de cores
    // Formato: ./simulador [política] [num_cores]
    // Exemplo: ./simulador fcfs 2
    if (argc >= 2) {
        string pol = argv[1];
        if      (pol == "fcfs")     policy = SchedPolicy::FCFS;
        else if (pol == "rr")       policy = SchedPolicy::RR;
        else if (pol == "priority") policy = SchedPolicy::PRIORITY;
        else if (pol == "sjn")      policy = SchedPolicy::SJN;
        else if (pol == "1" || pol == "2" || pol == "4") {
            // Se primeiro argumento é número, assume FCFS e número de cores
            NCORES = stoul(pol);
        }
    }
    
    // Segundo argumento pode ser número de cores
    if (argc >= 3) {
        try {
            size_t cores = stoul(argv[2]);
            if (cores > 0 && cores <= 8) {  // Limite razoável
                NCORES = cores;
            }
        } catch (...) {
            // Ignora se não for número válido
        }
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

    // Coletor de métricas temporais
    TemporalMetricsCollector temporalCollector(NCORES, RAM_SIZE);
    size_t completed_count = 0;
    uint64_t tick = 0;

    // ------------------------ CARREGAR PCBs ------------------------
    vector<unique_ptr<PCB>> allPCBs;
    vector<PCB*> pcbPtrs;

    uint64_t arrival_delay = 0;
    for (size_t i = 0; i < files.size(); i++) {
        auto up = make_unique<PCB>();

        if (!load_pcb_from_json(files[i], *up)) {
            cerr << "[main] Erro ao carregar " << files[i] << "\n";
            continue;
        }

        // Definir tempos de chegada diferentes para cada processo
        // Primeiro processo chega em 0, segundo em 2, terceiro em 4, etc.
        up->arrival_time = arrival_delay;
        arrival_delay += 2; // Delay de 2 ciclos entre chegadas

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
        // REQUISIÇÃO DE TAMANHO EM WORDS (cada elemento data/code já é contado em words)
        uint32_t req = p->data_bytes + p->code_bytes;
        if (req == 0) req = 1; // reservar ao menos 1 palavra

        Partition* part = memory.allocateFixedPartition(*p, req);

        if (!part) {
            pending.push_back(p);
            continue;
        }

        // DATA (cada índice é uma word index)
        for (uint32_t i = 0; i < p->data_bytes; i++)
            memory.writeLogical(i, p->dataSegment[i], *p);

        // CODE (base em words = data_bytes)
        uint32_t code_base = p->data_bytes;
        for (uint32_t i = 0; i < p->code_bytes; i++)
            memory.writeLogical(code_base + i, p->codeSegment[i], *p);

        // initial_pc em WORDS (PC aponta para início do código em índice de palavra)
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

                uint32_t code_base = p->data_bytes;
                for (uint32_t i = 0; i < p->code_bytes; i++)
                    memory.writeLogical(code_base + i, p->codeSegment[i], *p);

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
                completed_count++;
            }
            else if (ev.type == CoreEvent::BLOCKED) {
                ioManager.registerProcessWaitingForIO(ev.pcb, std::move(ev.ioRequests), 120);
            }
            else if (ev.type == CoreEvent::PREEMPTED) {
                scheduler.add(ev.pcb);
            }
        }

        ioManager.step();
        
        // Coletar métricas temporais (a cada 10 ticks para não gerar arquivo muito grande)
        if (tick % 10 == 0) {
            temporalCollector.collectSnapshot(tick, multicore, memory, completed_count);
        }
        
        tick++;
    }

    // ------------------------ FLUSH CACHE ------------------------
    for (auto &p : memory.L1_cache->dirtyData())
        memory.writeToFile(p.first, p.second);

    // ------------------------ MÉTRICAS ------------------------
    auto reports = Metrics::collect(allPCBs);
    auto core_reports = Metrics::collectCores(multicore.getCores());


    // Criar diretório output se não existir (no diretório de trabalho atual)
    try {
        std::filesystem::create_directories("output");
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[main] Aviso: Não foi possível criar diretório output: " << e.what() << "\n";
    }

    // Obter nome da política para salvar arquivos
    std::string policyName;
    switch (policy) {
        case SchedPolicy::FCFS: policyName = "fcfs"; break;
        case SchedPolicy::RR: policyName = "rr"; break;
        case SchedPolicy::PRIORITY: policyName = "priority"; break;
        case SchedPolicy::SJN: policyName = "sjn"; break;
    }
    
    // Criar subdiretório para esta política e número de cores
    std::string policyDir = "output/policies/" + policyName + "_" + std::to_string(NCORES) + "cores";
    try {
        std::filesystem::create_directories(policyDir);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[main] Aviso: Não foi possível criar diretório " << policyDir << ": " << e.what() << "\n";
    }

    // Salvar métricas básicas (com nome da política)
    Metrics::printConsole(reports);
    Metrics::saveCSV(reports, policyDir + "/metrics.csv");
    Metrics::saveJSON(reports, policyDir + "/metrics.json");

    // Calcular e salvar métricas agregadas por política
    auto policyMetrics = MetricsExtended::calculatePolicyMetrics(
        reports, policy, tick, NCORES
    );
    std::vector<MetricsExtended::PolicyMetrics> policyVec = {policyMetrics};
    MetricsExtended::printPolicyMetrics(policyVec);
    MetricsExtended::savePolicyMetricsCSV(policyVec, policyDir + "/policy_metrics.csv");

    // Salvar métricas temporais (evolução ao longo do tempo)
    temporalCollector.saveCSV(policyDir + "/temporal_metrics.csv");
    
    // Também salvar no diretório raiz para compatibilidade (última execução)
    Metrics::saveCSV(reports, "output/metrics.csv");
    MetricsExtended::savePolicyMetricsCSV(policyVec, "output/policy_metrics.csv");

    // Comparação Single-Core vs Multicore
    // Criar para qualquer número de cores (incluindo 1 core como baseline)
    MetricsExtended::CoreComparison core_comp;
    core_comp.num_cores = NCORES;
    if (!reports.empty()) {
        uint64_t total_wait = 0, total_turn = 0, total_service = 0;
        for (const auto& r : reports) {
            total_wait += r.waiting;
            total_turn += r.turnaround;
            total_service += r.pipeline_cycles;
        }
        core_comp.avg_waiting_time = (double)total_wait / reports.size();
        core_comp.avg_turnaround_time = (double)total_turn / reports.size();
        core_comp.cpu_utilization = tick > 0 && NCORES > 0 ? 
            ((double)total_service / (tick * NCORES)) * 100.0 : 0;
        core_comp.throughput = tick > 0 ? (double)reports.size() / tick : 0;
    }
    core_comp.speedup = 1.0; // Baseline (speedup real será calculado no script)
    
    std::vector<MetricsExtended::CoreComparison> comps = {core_comp};
    // Salvar por política e número de cores
    MetricsExtended::saveCoreComparisonCSV(comps, policyDir + "/core_comparison.csv");
    // E no diretório raiz para compatibilidade (apenas se > 1 core)
    if (NCORES > 1) {
        MetricsExtended::saveCoreComparisonCSV(comps, "output/core_comparison.csv");
    }

    cout << "\n[main] Simulação finalizada. Resultados salvos em output/:\n";
    cout << "  - metrics.csv (métricas por processo)\n";
    cout << "  - metrics.json (métricas por processo em JSON)\n";
    cout << "  - policy_metrics.csv (métricas agregadas por política)\n";
    cout << "  - temporal_metrics.csv (evolução temporal para gráficos)\n";
    if (NCORES > 1) {
        cout << "  - core_comparison.csv (comparação multicore)\n";
    }

    return 0;
}
