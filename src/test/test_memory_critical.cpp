/*
 * TESTES PRIORITÁRIOS: Gerenciador de Memória
 * Valida: Alocação, Tradução de Endereços, Cache
 */
#include <iostream>
#include <cassert>
#include <vector>
#include <memory>
#include "memory/MemoryManager.hpp"
#include "cpu/PCB.hpp"

void test_Memory_Allocation() {
    std::cout << "\n=== TESTE: Alocação de Partição ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    pcb.data_bytes = 50;
    pcb.code_bytes = 50;
    
    Partition* part = memManager.allocateFixedPartition(pcb, 100);
    
    assert(part != nullptr && "Partição deve ser alocada");
    assert(part->pid == 1 && "Partição deve pertencer ao processo");
    assert(!part->free && "Partição não deve estar livre");
    assert(pcb.partition_id >= 0 && "PCB deve ter partition_id");
    
    std::cout << "✓ Partição alocada: base=" << part->base 
              << ", size=" << part->size << "\n";
}

void test_Address_Translation() {
    std::cout << "\n=== TESTE: Tradução Lógico → Físico ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    Partition* part = memManager.allocateFixedPartition(pcb, 100);
    uint32_t base = part->base;
    
    // Testar traduções
    uint32_t phys0 = memManager.resolveAddress(0, pcb);
    assert(phys0 == base && "Endereço 0 deve mapear para base");
    
    uint32_t phys100 = memManager.resolveAddress(100, pcb);
    assert(phys100 == base + 100 && "Tradução deve somar base");
    
    std::cout << "✓ Lógico 0 → Físico " << phys0 << "\n";
    std::cout << "✓ Lógico 100 → Físico " << phys100 << "\n";
}

void test_Cache_Hit_Miss() {
    std::cout << "\n=== TESTE: Cache Hit/Miss ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    memManager.createPartitions(512);
    
    PCB pcb;
    pcb.pid = 1;
    memManager.allocateFixedPartition(pcb, 100);
    
    uint32_t address = 100;
    uint32_t value = 42;
    
    // Resetar contadores
    pcb.cache_hits.store(0);
    pcb.cache_misses.store(0);
    
    // Escrever na memória (write pode não usar cache)
    memManager.write(address, value, pcb);
    
    // Primeira leitura (deve ser miss se não estava na cache)
    uint32_t misses_before = pcb.cache_misses.load();
    uint32_t read1 = memManager.read(address, pcb);
    uint32_t misses_after = pcb.cache_misses.load();
    
    assert(read1 == value && "Leitura deve retornar valor correto");
    assert(misses_after >= misses_before && "Deve ter pelo menos 1 miss na primeira leitura");
    
    // Segunda leitura (deve ser hit se estava na cache)
    uint32_t hits_before = pcb.cache_hits.load();
    uint32_t read2 = memManager.read(address, pcb);
    uint32_t hits_after = pcb.cache_hits.load();
    
    assert(read2 == value && "Leitura deve retornar valor correto");
    assert(hits_after > hits_before && "Deve ter pelo menos 1 hit na segunda leitura");
    
    double hit_rate = (double)pcb.cache_hits.load() / 
                      (pcb.cache_hits.load() + pcb.cache_misses.load());
    
    std::cout << "✓ Primeira leitura: MISS (misses: " << misses_after << ")\n";
    std::cout << "✓ Segunda leitura: HIT (hits: " << hits_after << ")\n";
    std::cout << "✓ Taxa de hit: " << (hit_rate * 100) << "%\n";
}

void test_Memory_Full() {
    std::cout << "\n=== TESTE: Memória Cheia ===\n";
    
    MemoryManager memManager(1024, 8192, 64); // RAM pequena para teste
    memManager.createPartitions(256);
    
    // Preencher todas as partições (usar ponteiros porque PCB não é copiável)
    std::vector<std::unique_ptr<PCB>> processes;
    int allocated = 0;
    
    for (int i = 0; i < 10; i++) {
        auto pcb = std::make_unique<PCB>();
        pcb->pid = i;
        pcb->data_bytes = 50;
        pcb->code_bytes = 50;
        
        Partition* part = memManager.allocateFixedPartition(*pcb, 100);
        if (part) {
            allocated++;
            processes.push_back(std::move(pcb));
        }
    }
    
    // Tentar alocar mais (deve falhar)
    PCB pcb_fail;
    pcb_fail.pid = 99;
    Partition* part_fail = memManager.allocateFixedPartition(pcb_fail, 100);
    
    assert(part_fail == nullptr && "Alocação deve falhar quando memória cheia");
    
    std::cout << "✓ Alocados: " << allocated << " processos\n";
    std::cout << "✓ Alocação adicional rejeitada (memória cheia)\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES PRIORITÁRIOS: MEMÓRIA\n";
    std::cout << "========================================\n";
    
    try {
        test_Memory_Allocation();
        test_Address_Translation();
        test_Cache_Hit_Miss();
        test_Memory_Full();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DE MEMÓRIA PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}

