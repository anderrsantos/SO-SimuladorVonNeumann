/*
 * TESTES DE MÉTRICAS
 * Valida: Contabilização correta de métricas do sistema
 */
#include <iostream>
#include <cassert>
#include "multicore/MultiCore.hpp"
#include "multicore/Scheduler.hpp"
#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp"
#include "cpu/PCB.hpp"
#include "metrics/Metrics.hpp"

void test_PCB_Metrics() {
    std::cout << "\n=== TESTE: Métricas do PCB ===\n";
    
    PCB pcb;
    pcb.pid = 1;
    pcb.arrival_time = 0;
    pcb.start_time = 10;
    pcb.finish_time = 100;
    
    // Simular algumas operações
    pcb.pipeline_cycles.store(50);
    pcb.cache_hits.store(30);
    pcb.cache_misses.store(20);
    pcb.memory_cycles.store(150);
    pcb.io_cycles.store(25);
    
    // Calcular métricas
    uint64_t turnaround = pcb.finish_time - pcb.arrival_time;
    uint64_t response = pcb.start_time - pcb.arrival_time;
    uint64_t service = pcb.pipeline_cycles.load();
    uint64_t wait = (turnaround > service ? turnaround - service : 0);
    
    double hit_rate = (double)pcb.cache_hits.load() / 
                      (pcb.cache_hits.load() + pcb.cache_misses.load());
    
    std::cout << "  Turnaround time: " << turnaround << "\n";
    std::cout << "  Response time: " << response << "\n";
    std::cout << "  Service time: " << service << "\n";
    std::cout << "  Wait time: " << wait << "\n";
    std::cout << "  Cache hit rate: " << (hit_rate * 100) << "%\n";
    std::cout << "  Memory cycles: " << pcb.memory_cycles.load() << "\n";
    std::cout << "  I/O cycles: " << pcb.io_cycles.load() << "\n";
    
    assert(turnaround == 100 && "Turnaround time correto");
    assert(response == 10 && "Response time correto");
    assert(hit_rate > 0.5 && "Hit rate razoável");
    
    std::cout << "✓ Métricas do PCB calculadas corretamente\n";
}

void test_Memory_Metrics() {
    std::cout << "\n=== TESTE: Métricas de Memória ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    memManager.allocateFixedPartition(pcb, 100);
    
    // Resetar contadores
    pcb.cache_hits.store(0);
    pcb.cache_misses.store(0);
    pcb.primary_mem_accesses.store(0);
    pcb.secondary_mem_accesses.store(0);
    pcb.memory_cycles.store(0);
    
    // Fazer vários acessos
    for (int i = 0; i < 50; i++) {
        uint32_t addr = 100 + (i * 4);
        memManager.read(addr, pcb);
    }
    
    // Escrever alguns valores
    for (int i = 0; i < 10; i++) {
        uint32_t addr = 200 + (i * 4);
        memManager.write(addr, i, pcb);
    }
    
    std::cout << "  Cache hits: " << pcb.cache_hits.load() << "\n";
    std::cout << "  Cache misses: " << pcb.cache_misses.load() << "\n";
    std::cout << "  Primary mem accesses: " << pcb.primary_mem_accesses.load() << "\n";
    std::cout << "  Secondary mem accesses: " << pcb.secondary_mem_accesses.load() << "\n";
    std::cout << "  Memory cycles: " << pcb.memory_cycles.load() << "\n";
    std::cout << "  Mem reads: " << pcb.mem_reads.load() << "\n";
    std::cout << "  Mem writes: " << pcb.mem_writes.load() << "\n";
    
    assert(pcb.cache_hits.load() + pcb.cache_misses.load() > 0 && "Deve haver acessos");
    assert(pcb.mem_reads.load() == 50 && "Deve ter 50 leituras");
    assert(pcb.mem_writes.load() == 10 && "Deve ter 10 escritas");
    assert(pcb.memory_cycles.load() > 0 && "Deve ter ciclos de memória");
    
    std::cout << "✓ Métricas de memória contabilizadas corretamente\n";
}

void test_Pipeline_Metrics() {
    std::cout << "\n=== TESTE: Métricas do Pipeline ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    IOManager ioManager;
    bool printLock = false;
    Core core(0, &memManager, &ioManager, &printLock);
    
    PCB pcb;
    pcb.pid = 1;
    pcb.quantum = 100;
    
    memManager.createPartitions(512);
    memManager.allocateFixedPartition(pcb, 100);
    
    // Resetar contadores
    pcb.pipeline_cycles.store(0);
    pcb.stage_invocations.store(0);
    
    core.assignProcess(&pcb);
    
    // Executar alguns ciclos
    int cycles = 0;
    while (cycles < 20) {
        CoreEvent ev = core.stepOneCycle();
        cycles++;
        if (ev.type == CoreEvent::FINISHED || ev.type == CoreEvent::PREEMPTED) {
            break;
        }
    }
    
    std::cout << "  Pipeline cycles: " << pcb.pipeline_cycles.load() << "\n";
    std::cout << "  Stage invocations: " << pcb.stage_invocations.load() << "\n";
    std::cout << "  Ciclos executados: " << cycles << "\n";
    
    assert(pcb.pipeline_cycles.load() > 0 && "Deve ter ciclos de pipeline");
    assert(pcb.pipeline_cycles.load() == cycles && "Ciclos devem coincidir");
    
    std::cout << "✓ Métricas do pipeline contabilizadas corretamente\n";
}

void test_System_Metrics() {
    std::cout << "\n=== TESTE: Métricas do Sistema ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    IOManager ioManager;
    Scheduler scheduler(SchedPolicy::FCFS);
    MultiCore multicore(2, &memManager, &ioManager, nullptr);
    
    std::vector<std::unique_ptr<PCB>> allPCBs;
    
    // Criar alguns processos
    for (int i = 0; i < 3; i++) {
        auto pcb = std::make_unique<PCB>();
        pcb->pid = i + 1;
        pcb->arrival_time = i * 10;
        pcb->quantum = 20;
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
    
    // Executar
    uint64_t tick = 0;
    while (!scheduler.empty() && tick < 100) {
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
            } else if (ev.type == CoreEvent::PREEMPTED) {
                scheduler.add(ev.pcb);
            }
        }
        
        tick++;
    }
    
    // Coletar métricas
    std::vector<PCB*> pcbPtrs;
    for (auto& pcb : allPCBs) {
        pcbPtrs.push_back(pcb.get());
    }
    
    auto reports = Metrics::collect(allPCBs);
    
    std::cout << "  Processos processados: " << allPCBs.size() << "\n";
    std::cout << "  Ciclos totais: " << tick << "\n";
    std::cout << "  Relatórios gerados: " << reports.size() << "\n";
    
    assert(reports.size() > 0 && "Deve gerar relatórios");
    std::cout << "✓ Métricas do sistema coletadas\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES DE MÉTRICAS\n";
    std::cout << "========================================\n";
    
    try {
        test_PCB_Metrics();
        test_Memory_Metrics();
        test_Pipeline_Metrics();
        test_System_Metrics();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DE MÉTRICAS PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}





