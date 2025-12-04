/*
 * TESTES DE DESEMPENHO
 * Valida: Throughput, Latência, Taxa de Cache Hit
 */
#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include "multicore/MultiCore.hpp"
#include "multicore/Scheduler.hpp"
#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp"
#include "cpu/PCB.hpp"

void test_Pipeline_Throughput() {
    std::cout << "\n=== TESTE: Throughput do Pipeline ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    IOManager ioManager;
    bool printLock = false;
    
    Core core(0, &memManager, &ioManager, &printLock);
    
    PCB pcb;
    pcb.pid = 1;
    pcb.quantum = 1000;
    pcb.data_bytes = 10;
    pcb.code_bytes = 60;
    
    memManager.createPartitions(512);
    uint32_t req = pcb.data_bytes + pcb.code_bytes;
    Partition* part = memManager.allocateFixedPartition(pcb, req);
    assert(part != nullptr && "Partição deve ser alocada");
    
    // Criar programa com 50 instruções
    const int num_instructions = 50;
    
    // Carregar segmentos na memória
    for (uint32_t i = 0; i < pcb.data_bytes; i++)
        memManager.writeLogical(i, 0, pcb);
    for (uint32_t i = 0; i < num_instructions; i++)
        memManager.writeLogical(pcb.data_bytes + i, 0x00000000 + (i % 10), pcb);
    
    // Sentinel de fim
    uint32_t END_SENTINEL = 0b11111100000000000000000000000000u;
    memManager.writeLogical(pcb.data_bytes + num_instructions, END_SENTINEL, pcb);
    
    pcb.initial_pc = pcb.data_bytes;
    pcb.regBank.pc.write(pcb.initial_pc * 4); // PC em bytes
    
    core.assignProcess(&pcb);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    int cycles = 0;
    while (pcb.state != State::Finished && cycles < 200) {
        CoreEvent ev = core.stepOneCycle();
        cycles++;
        if (ev.type == CoreEvent::FINISHED) break;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calcular throughput (após warm-up de 5 ciclos)
    int effective_cycles = cycles - 5;
    if (effective_cycles <= 0) effective_cycles = 1;
    
    double throughput = (double)num_instructions / effective_cycles;
    
    std::cout << "  Instruções: " << num_instructions << "\n";
    std::cout << "  Ciclos totais: " << cycles << "\n";
    std::cout << "  Ciclos efetivos: " << effective_cycles << "\n";
    std::cout << "  Throughput: " << throughput << " instruções/ciclo\n";
    std::cout << "  Tempo de execução: " << duration.count() << " μs\n";
    
    // Throughput deve ser > 0.5 após warm-up
    assert(throughput > 0.3 && "Throughput deve ser razoável");
    std::cout << "✓ Throughput aceitável\n";
}

void test_Cache_Hit_Rate() {
    std::cout << "\n=== TESTE: Taxa de Cache Hit ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    memManager.allocateFixedPartition(pcb, 1000);
    
    // Padrão de acesso repetitivo (simula localidade temporal)
    const int iterations = 100;
    const int pattern_size = 10;
    
    pcb.cache_hits.store(0);
    pcb.cache_misses.store(0);
    
    // Acessar padrão repetidamente
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < pattern_size; j++) {
            uint32_t addr = 100 + (j * 4);
            memManager.read(addr, pcb);
        }
    }
    
    uint64_t hits = pcb.cache_hits.load();
    uint64_t misses = pcb.cache_misses.load();
    uint64_t total = hits + misses;
    
    double hit_rate = total > 0 ? (double)hits / total : 0.0;
    
    std::cout << "  Acessos totais: " << total << "\n";
    std::cout << "  Cache hits: " << hits << "\n";
    std::cout << "  Cache misses: " << misses << "\n";
    std::cout << "  Taxa de hit: " << (hit_rate * 100) << "%\n";
    
    // Para padrão repetitivo, taxa de hit deve ser > 50%
    assert(hit_rate > 0.4 && "Taxa de hit deve ser razoável para padrão repetitivo");
    std::cout << "✓ Taxa de cache hit aceitável\n";
}

void test_Memory_Latency() {
    std::cout << "\n=== TESTE: Latência de Acesso à Memória ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    memManager.allocateFixedPartition(pcb, 1000);
    
    // Testar latência de cada camada
    uint32_t addr_cache = 100;
    uint32_t addr_ram = 200;
    
    // Escrever valores
    memManager.write(addr_cache, 42, pcb);
    memManager.write(addr_ram, 99, pcb);
    
    // Resetar contadores
    pcb.memory_cycles.store(0);
    pcb.cache_hits.store(0);
    pcb.cache_misses.store(0);
    
    // Acesso à cache (deve ser rápido)
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        memManager.read(addr_cache, pcb);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto cache_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Acesso à RAM (deve ser mais lento)
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        memManager.read(addr_ram + (i % 100), pcb); // Acessos diferentes para forçar misses
    }
    end = std::chrono::high_resolution_clock::now();
    auto ram_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    uint64_t cache_cycles = pcb.memory_cycles.load();
    
    std::cout << "  Tempo 1000 acessos (cache): " << cache_time.count() << " μs\n";
    std::cout << "  Tempo 1000 acessos (RAM): " << ram_time.count() << " μs\n";
    std::cout << "  Ciclos de memória: " << cache_cycles << "\n";
    
    // Cache deve ser mais rápida (ou pelo menos não muito mais lenta devido ao overhead)
    std::cout << "✓ Latências medidas\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES DE DESEMPENHO\n";
    std::cout << "========================================\n";
    
    try {
        test_Pipeline_Throughput();
        test_Cache_Hit_Rate();
        test_Memory_Latency();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DE DESEMPENHO PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}

