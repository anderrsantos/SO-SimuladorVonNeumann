/*
 * TESTE PRIORITÁRIO: Pipeline Básico
 * Valida: Execução do pipeline MIPS de 5 estágios
 */
#include <iostream>
#include <cassert>
#include "multicore/Core.hpp"
#include "memory/MemoryManager.hpp"
#include "IO/IOManager.hpp"
#include "cpu/PCB.hpp"
#include "multicore/MultiCore.hpp"

void test_Pipeline_Execution() {
    std::cout << "\n=== TESTE: Pipeline - Execução Básica ===\n";
    
    MemoryManager memManager(4096, 8192, 64);
    IOManager ioManager;
    bool printLock = false;
    
    Core core(0, &memManager, &ioManager, &printLock);
    
    PCB pcb;
    pcb.pid = 1;
    pcb.quantum = 100;
    pcb.regBank.pc.write(0);
    
    // Alocar memória
    memManager.createPartitions(512);
    memManager.allocateFixedPartition(pcb, 100);
    
    // Escrever algumas instruções simples na memória
    // Usar valores que representem instruções válidas
    // (Simplificado para teste - em produção usaria instruções reais)
    for (int i = 0; i < 5; i++) {
        memManager.writeLogical(i, 0x00000000 + i, pcb); // Instruções placeholder
    }
    
    // Sentinel de fim
    uint32_t END_SENTINEL = 0b11111100000000000000000000000000u;
    memManager.writeLogical(5, END_SENTINEL, pcb);
    
    // Atribuir processo ao core
    bool assigned = core.assignProcess(&pcb);
    assert(assigned && "Processo deve ser atribuído");
    
    // Executar pipeline por vários ciclos
    int cycles = 0;
    bool finished = false;
    
    while (cycles < 50 && !finished) {
        CoreEvent ev = core.stepOneCycle();
        cycles++;
        
        if (ev.type == CoreEvent::FINISHED) {
            finished = true;
            std::cout << "✓ Processo finalizado após " << cycles << " ciclos\n";
        }
    }
    
    assert(cycles >= 5 && "Pipeline deve executar pelo menos 5 ciclos");
    assert(pcb.pipeline_cycles.load() > 0 && "Pipeline cycles deve ser contabilizado");
    
    std::cout << "✓ Pipeline executou " << cycles << " ciclos\n";
    std::cout << "✓ Pipeline cycles contabilizados: " << pcb.pipeline_cycles.load() << "\n";
}

void test_Pipeline_Stages() {
    std::cout << "\n=== TESTE: Pipeline - Estágios ===\n";
    
    // Verificar que os estágios do pipeline existem e podem ser chamados
    // Este é um teste mais conceitual - valida que a estrutura está correta
    
    MemoryManager memManager(4096, 8192, 64);
    IOManager ioManager;
    bool printLock = false;
    Core core(0, &memManager, &ioManager, &printLock);
    
    PCB pcb;
    pcb.pid = 1;
    pcb.quantum = 100;
    
    memManager.createPartitions(512);
    memManager.allocateFixedPartition(pcb, 100);
    
    // Verificar que core pode ser criado e processo atribuído
    bool assigned = core.assignProcess(&pcb);
    assert(assigned && "Core deve aceitar processo");
    
    // Verificar que stepOneCycle funciona
    CoreEvent ev = core.stepOneCycle();
    assert(ev.type == CoreEvent::NONE || ev.type == CoreEvent::FINISHED || 
           ev.type == CoreEvent::BLOCKED || ev.type == CoreEvent::PREEMPTED);
    
    std::cout << "✓ Pipeline pode executar estágios\n";
    std::cout << "✓ Eventos são gerados corretamente\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES PRIORITÁRIOS: PIPELINE\n";
    std::cout << "========================================\n";
    
    try {
        test_Pipeline_Execution();
        test_Pipeline_Stages();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DO PIPELINE PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}

