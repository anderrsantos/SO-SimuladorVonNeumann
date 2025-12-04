/*
 * TESTES PRIORITÁRIOS: Casos de Borda Críticos
 * Valida: Comportamento em situações extremas
 */
#include <iostream>
#include <cassert>
#include "multicore/Scheduler.hpp"
#include "memory/MemoryManager.hpp"
#include "cpu/PCB.hpp"

void test_Empty_Scheduler() {
    std::cout << "\n=== TESTE: Scheduler Vazio ===\n";
    
    Scheduler scheduler(SchedPolicy::FCFS);
    
    assert(scheduler.empty() && "Scheduler vazio deve retornar empty()=true");
    assert(scheduler.fetchNext() == nullptr && "fetchNext() deve retornar nullptr");
    
    std::cout << "✓ Scheduler vazio tratado corretamente\n";
}

void test_Memory_Protection() {
    std::cout << "\n=== TESTE: Proteção de Memória ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    Partition* part = memManager.allocateFixedPartition(pcb, 100);
    
    // Tentar acessar endereço dentro da partição (deve funcionar)
    try {
        uint32_t addr_valid = part->size - 1;
        uint32_t phys = memManager.resolveAddress(addr_valid, pcb);
        std::cout << "✓ Acesso válido: endereço " << addr_valid << " → " << phys << "\n";
    } catch (...) {
        assert(false && "Acesso válido não deve lançar exceção");
    }
    
    // Tentar acessar endereço fora da partição (deve lançar exceção)
    bool exception_thrown = false;
    try {
        uint32_t addr_invalid = part->size; // Fora dos limites
        memManager.resolveAddress(addr_invalid, pcb);
    } catch (const std::out_of_range& e) {
        exception_thrown = true;
        std::cout << "✓ Acesso inválido detectado: " << e.what() << "\n";
    } catch (...) {
        // Outra exceção também é aceitável
        exception_thrown = true;
        std::cout << "✓ Acesso inválido detectado (exceção genérica)\n";
    }
    
    assert(exception_thrown && "Acesso inválido deve lançar exceção");
}

void test_Cache_Eviction() {
    std::cout << "\n=== TESTE: Cache - Eviction FIFO ===\n";
    
    MemoryManager memManager(4096, 8192, 3); // Cache pequena (capacity=3)
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    memManager.allocateFixedPartition(pcb, 100);
    
    // Preencher cache
    memManager.write(100, 1, pcb);
    memManager.write(200, 2, pcb);
    memManager.write(300, 3, pcb);
    
    // Ler para garantir que estão na cache
    memManager.read(100, pcb);
    memManager.read(200, pcb);
    memManager.read(300, pcb);
    
    // Inserir novo (deve evictar o primeiro)
    memManager.write(400, 4, pcb);
    memManager.read(400, pcb);
    
    // Tentar ler o primeiro (deve ser miss agora)
    uint64_t misses_before = pcb.cache_misses.load();
    memManager.read(100, pcb);
    uint64_t misses_after = pcb.cache_misses.load();
    
    // Deve ter incrementado misses (eviction funcionou)
    assert(misses_after > misses_before && "Eviction deve ter ocorrido");
    
    std::cout << "✓ Cache eviction funcionando (FIFO)\n";
    std::cout << "  Misses antes: " << misses_before 
              << ", depois: " << misses_after << "\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES PRIORITÁRIOS: CASOS DE BORDA\n";
    std::cout << "========================================\n";
    
    try {
        test_Empty_Scheduler();
        test_Memory_Protection();
        test_Cache_Eviction();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DE CASOS DE BORDA PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}

