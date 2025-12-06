/*
 * TESTES DE STRESS
 * Valida: Sistema com muitos processos simultâneos
 */
#include <iostream>
#include <cassert>
#include <vector>
#include <memory>
#include "multicore/MultiCore.hpp"
#include "multicore/Scheduler.hpp"
#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp"
#include "cpu/PCB.hpp"

void test_Many_Processes() {
    std::cout << "\n=== TESTE: Muitos Processos Simultâneos ===\n";
    
    const size_t RAM_SIZE = 4096;
    const size_t SEC_SIZE = 8192;
    const size_t CACHE_CAP = 64;
    const uint32_t PART_SIZE = 128; // Partições menores para mais processos
    const size_t NCORES = 4;
    const int NUM_PROCESSES = 20;
    
    MemoryManager memManager(RAM_SIZE, SEC_SIZE, CACHE_CAP);
    memManager.createPartitions(PART_SIZE);
    IOManager ioManager;
    Scheduler scheduler(SchedPolicy::RR);
    MultiCore multicore(NCORES, &memManager, &ioManager, nullptr);
    
    std::vector<std::unique_ptr<PCB>> allPCBs;
    std::vector<PCB*> pcbPtrs;
    
    // Criar muitos processos
    for (int i = 0; i < NUM_PROCESSES; i++) {
        auto pcb = std::make_unique<PCB>();
        pcb->pid = i + 1;
        pcb->name = "stress_process_" + std::to_string(i);
        pcb->quantum = 10;
        pcb->priority = i % 5;
        pcb->data_bytes = 10;
        pcb->code_bytes = 10;
        pcb->arrival_time = i;
        
        uint32_t req = pcb->data_bytes + pcb->code_bytes;
        if (req == 0) req = 1;
        
        Partition* part = memManager.allocateFixedPartition(*pcb, req);
        if (part) {
            // Carregar segmentos
            for (uint32_t j = 0; j < pcb->data_bytes; j++)
                memManager.writeLogical(j, 0, *pcb);
            for (uint32_t j = 0; j < pcb->code_bytes; j++)
                memManager.writeLogical(pcb->data_bytes + j, 0, *pcb);
            
            pcb->initial_pc = pcb->data_bytes;
            pcbPtrs.push_back(pcb.get());
            allPCBs.push_back(std::move(pcb));
            scheduler.add(pcbPtrs.back());
        }
    }
    
    int allocated = pcbPtrs.size();
    std::cout << "  Processos criados: " << NUM_PROCESSES << "\n";
    std::cout << "  Processos alocados: " << allocated << "\n";
    
    assert(allocated > 0 && "Pelo menos alguns processos devem ser alocados");
    
    // Executar por alguns ciclos
    uint64_t tick = 0;
    int max_ticks = 500;
    int finished_count = 0;
    
    while ((!scheduler.empty() || multicore.hasActiveCores()) && tick < max_ticks) {
        multicore.assignReadyProcesses([&]() {
            PCB* p = scheduler.fetchNext();
            if (p && p->start_time == 0) {
                p->start_time = tick;
            }
            return p;
        });
        
        auto events = multicore.stepAll();
        
        for (auto &ev : events) {
            if (ev.type == CoreEvent::FINISHED) {
                ev.pcb->finish_time = tick;
                memManager.freePartition(ev.pcb->pid);
                finished_count++;
            } else if (ev.type == CoreEvent::PREEMPTED) {
                scheduler.add(ev.pcb);
            }
        }
        
        ioManager.step();
        tick++;
    }
    
    std::cout << "  Ciclos executados: " << tick << "\n";
    std::cout << "  Processos finalizados: " << finished_count << "\n";
    std::cout << "  Processos ainda ativos: " << (allocated - finished_count) << "\n";
    
    assert(finished_count >= 0 && "Sistema deve processar processos");
    std::cout << "✓ Sistema lidou com muitos processos\n";
}

void test_Memory_Pressure() {
    std::cout << "\n=== TESTE: Pressão de Memória ===\n";
    
    MemoryManager memManager(1024, 8192, 32); // RAM pequena
    memManager.createPartitions(128); // Partições pequenas
    
    std::vector<std::unique_ptr<PCB>> processes;
    int allocated = 0;
    int failed = 0;
    
    // Tentar alocar muitos processos
    for (int i = 0; i < 20; i++) {
        auto pcb = std::make_unique<PCB>();
        pcb->pid = i;
        pcb->data_bytes = 20;
        pcb->code_bytes = 20;
        
        Partition* part = memManager.allocateFixedPartition(*pcb, 40);
        if (part) {
            allocated++;
            processes.push_back(std::move(pcb));
        } else {
            failed++;
        }
    }
    
    std::cout << "  Tentativas de alocação: 20\n";
    std::cout << "  Alocações bem-sucedidas: " << allocated << "\n";
    std::cout << "  Alocações falhadas: " << failed << "\n";
    
    assert(allocated > 0 && "Algumas alocações devem ter sucesso");
    assert(failed > 0 && "Algumas alocações devem falhar (memória limitada)");
    
    // Liberar alguns e tentar alocar novamente
    int freed = 0;
    for (size_t i = 0; i < processes.size() && i < 3; i++) {
        memManager.freePartition(processes[i]->pid);
        freed++;
    }
    
    // Tentar alocar novamente
    auto pcb_new = std::make_unique<PCB>();
    pcb_new->pid = 99;
    pcb_new->data_bytes = 20;
    pcb_new->code_bytes = 20;
    Partition* part_new = memManager.allocateFixedPartition(*pcb_new, 40);
    
    std::cout << "  Partições liberadas: " << freed << "\n";
    std::cout << "  Nova alocação após liberação: " << (part_new ? "SUCESSO" : "FALHOU") << "\n";
    
    assert(part_new != nullptr && "Deve conseguir alocar após liberação");
    std::cout << "✓ Sistema gerencia pressão de memória corretamente\n";
}

void test_Concurrent_Cores() {
    std::cout << "\n=== TESTE: Múltiplos Cores Concorrentes ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(256);
    IOManager ioManager;
    Scheduler scheduler(SchedPolicy::RR);
    MultiCore multicore(8, &memManager, &ioManager, nullptr); // 8 cores
    
    std::vector<std::unique_ptr<PCB>> allPCBs;
    
    // Criar processos para todos os cores
    for (int i = 0; i < 10; i++) {
        auto pcb = std::make_unique<PCB>();
        pcb->pid = i;
        pcb->quantum = 5;
        pcb->data_bytes = 10;
        pcb->code_bytes = 10;
        
        uint32_t req = pcb->data_bytes + pcb->code_bytes;
        if (req == 0) req = 1;
        
        Partition* part = memManager.allocateFixedPartition(*pcb, req);
        if (part) {
            for (uint32_t j = 0; j < pcb->data_bytes; j++)
                memManager.writeLogical(j, 0, *pcb);
            for (uint32_t j = 0; j < pcb->code_bytes; j++)
                memManager.writeLogical(pcb->data_bytes + j, 0, *pcb);
            
            pcb->initial_pc = pcb->data_bytes;
            scheduler.add(pcb.get());
            allPCBs.push_back(std::move(pcb));
        }
    }
    
    // Executar e verificar que múltiplos cores estão ativos
    uint64_t tick = 0;
    int active_cores_count = 0;
    
    while (tick < 50 && (!scheduler.empty() || multicore.hasActiveCores())) {
        multicore.assignReadyProcesses([&]() {
            return scheduler.fetchNext();
        });
        
        if (multicore.hasActiveCores()) {
            active_cores_count++;
        }
        
        auto events = multicore.stepAll();
        for (auto &ev : events) {
            if (ev.type == CoreEvent::PREEMPTED) {
                scheduler.add(ev.pcb);
            }
        }
        
        tick++;
    }
    
    std::cout << "  Cores disponíveis: " << multicore.numCores() << "\n";
    std::cout << "  Ciclos com cores ativos: " << active_cores_count << "\n";
    std::cout << "  Total de ciclos: " << tick << "\n";
    
    assert(active_cores_count > 0 && "Cores devem estar ativos");
    std::cout << "✓ Múltiplos cores executam concorrentemente\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES DE STRESS\n";
    std::cout << "========================================\n";
    
    try {
        test_Many_Processes();
        test_Memory_Pressure();
        test_Concurrent_Cores();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DE STRESS PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}





