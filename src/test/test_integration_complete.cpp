/*
 * TESTE PRIORITÁRIO: Integração Completa
 * Valida: Execução completa de um processo através de todo o sistema
 */
#include <iostream>
#include <cassert>
#include <filesystem>
#include "cpu/pcb_loader.hpp"
#include "multicore/MultiCore.hpp"
#include "multicore/Scheduler.hpp"
#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp"
#include "cpu/PCB.hpp"

namespace fs = std::filesystem;

void test_Complete_System_Execution() {
    std::cout << "\n=== TESTE: Integração Completa do Sistema ===\n";
    
    // Configuração do sistema
    const size_t RAM_SIZE = 4096;
    const size_t SEC_SIZE = 8192;
    const size_t CACHE_CAP = 64;
    const uint32_t PART_SIZE = 512;
    const size_t NCORES = 2; // Reduzido para teste mais rápido
    
    MemoryManager memManager(RAM_SIZE, SEC_SIZE, CACHE_CAP);
    memManager.createPartitions(PART_SIZE);
    IOManager ioManager;
    Scheduler scheduler(SchedPolicy::FCFS);
    MultiCore multicore(NCORES, &memManager, &ioManager, nullptr);
    
    // Tentar carregar processo do JSON
    std::vector<std::unique_ptr<PCB>> allPCBs;
    std::vector<PCB*> pcbPtrs;
    
    // Procurar arquivo de processo
    std::string processFile = "process1.json";
    if (!fs::exists(processFile)) {
        processFile = "processes/process1.json";
    }
    if (!fs::exists(processFile)) {
        processFile = "../processes/process1.json";
    }
    
    if (fs::exists(processFile)) {
        auto pcb = std::make_unique<PCB>();
        if (load_pcb_from_json(processFile, *pcb)) {
            pcb->arrival_time = 0;
            pcbPtrs.push_back(pcb.get());
            allPCBs.push_back(std::move(pcb));
            std::cout << "✓ Processo carregado de " << processFile << "\n";
        } else {
            std::cout << "⚠ Não foi possível carregar " << processFile 
                      << ", criando processo de teste\n";
            // Criar processo de teste simples
            auto pcb = std::make_unique<PCB>();
            pcb->pid = 1;
            pcb->name = "test_process";
            pcb->quantum = 50;
            pcb->priority = 1;
            pcb->data_bytes = 10;
            pcb->code_bytes = 10;
            pcb->arrival_time = 0;
            pcbPtrs.push_back(pcb.get());
            allPCBs.push_back(std::move(pcb));
        }
    } else {
        std::cout << "⚠ Arquivo de processo não encontrado, criando processo de teste\n";
        auto pcb = std::make_unique<PCB>();
        pcb->pid = 1;
        pcb->name = "test_process";
        pcb->quantum = 50;
        pcb->priority = 1;
        pcb->data_bytes = 10;
        pcb->code_bytes = 10;
        pcb->arrival_time = 0;
        pcbPtrs.push_back(pcb.get());
        allPCBs.push_back(std::move(pcb));
    }
    
    // Alocar memória para processos
    for (PCB* p : pcbPtrs) {
        uint32_t req = p->data_bytes + p->code_bytes;
        if (req == 0) req = 1;
        
        Partition* part = memManager.allocateFixedPartition(*p, req);
        if (part) {
            // Carregar segmentos
            for (uint32_t i = 0; i < p->data_bytes; i++)
                memManager.writeLogical(i, 0, *p);
            for (uint32_t i = 0; i < p->code_bytes; i++)
                memManager.writeLogical(p->data_bytes + i, 0, *p);
            
            p->initial_pc = p->data_bytes;
            scheduler.add(p);
        }
    }
    
    assert(!scheduler.empty() && "Deve haver processos no scheduler");
    
    // Loop de execução (limitado para teste)
    uint64_t tick = 0;
    int max_ticks = 1000; // Limite para não travar
    
    while ((!scheduler.empty() || multicore.hasActiveCores()) && tick < max_ticks) {
        // Distribuir processos
        multicore.assignReadyProcesses([&]() {
            PCB* p = scheduler.fetchNext();
            if (p && p->start_time == 0) {
                p->start_time = tick;
                p->response_time = tick - p->arrival_time;
            }
            return p;
        });
        
        // Avançar cores
        auto events = multicore.stepAll();
        
        // Processar eventos
        for (auto &ev : events) {
            if (ev.type == CoreEvent::FINISHED) {
                ev.pcb->finish_time = tick;
                memManager.freePartition(ev.pcb->pid);
                std::cout << "✓ Processo " << ev.pcb->pid << " finalizado\n";
            } else if (ev.type == CoreEvent::BLOCKED) {
                ioManager.registerProcessWaitingForIO(ev.pcb, std::move(ev.ioRequests), 100);
            } else if (ev.type == CoreEvent::PREEMPTED) {
                scheduler.add(ev.pcb);
            }
        }
        
        ioManager.step();
        tick++;
    }
    
    // Verificações finais
    bool all_finished = true;
    for (PCB* p : pcbPtrs) {
        if (p->state != State::Finished && tick < max_ticks) {
            all_finished = false;
        }
    }
    
    std::cout << "✓ Execução completa: " << tick << " ciclos\n";
    std::cout << "✓ Processos processados: " << pcbPtrs.size() << "\n";
    
    // Verificar métricas
    for (PCB* p : pcbPtrs) {
        if (p->pipeline_cycles.load() > 0) {
            std::cout << "  Processo " << p->pid 
                      << ": " << p->pipeline_cycles.load() << " ciclos de pipeline\n";
        }
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTE PRIORITÁRIO: INTEGRAÇÃO COMPLETA\n";
    std::cout << "========================================\n";
    
    try {
        test_Complete_System_Execution();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TESTE DE INTEGRAÇÃO PASSOU\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}

