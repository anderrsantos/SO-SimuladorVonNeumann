/*
 * TESTE PRIORITÁRIO: Escalonador - Todas as Políticas
 * Valida: FCFS, Round-Robin, Priority, SJN
 */
#include <iostream>
#include <cassert>
#include "multicore/Scheduler.hpp"
#include "cpu/PCB.hpp"

void test_FCFS() {
    std::cout << "\n=== TESTE: FCFS - Ordem de Chegada ===\n";
    
    Scheduler scheduler(SchedPolicy::FCFS);
    
    PCB p1, p2, p3;
    p1.pid = 1; p1.priority = 10; // Alta prioridade (não deve importar)
    p2.pid = 2; p2.priority = 1;
    p3.pid = 3; p3.priority = 5;
    
    scheduler.add(&p1);
    scheduler.add(&p2);
    scheduler.add(&p3);
    
    assert(scheduler.fetchNext()->pid == 1);
    assert(scheduler.fetchNext()->pid == 2);
    assert(scheduler.fetchNext()->pid == 3);
    assert(scheduler.empty());
    
    std::cout << "✓ FCFS: Processos executam na ordem de chegada\n";
}

void test_RoundRobin() {
    std::cout << "\n=== TESTE: Round-Robin - Rotação ===\n";
    
    Scheduler scheduler(SchedPolicy::RR);
    
    PCB p1, p2, p3;
    p1.pid = 1; p1.quantum = 3;
    p2.pid = 2; p2.quantum = 3;
    p3.pid = 3; p3.quantum = 3;
    
    scheduler.add(&p1);
    scheduler.add(&p2);
    scheduler.add(&p3);
    
    // Deve retornar na ordem: P1, P2, P3
    assert(scheduler.fetchNext()->pid == 1);
    assert(scheduler.fetchNext()->pid == 2);
    assert(scheduler.fetchNext()->pid == 3);
    
    std::cout << "✓ Round-Robin: Processos em fila circular\n";
}

void test_Priority() {
    std::cout << "\n=== TESTE: Priority - Maior Prioridade Primeiro ===\n";
    
    Scheduler scheduler(SchedPolicy::PRIORITY);
    
    PCB p1, p2, p3;
    p1.pid = 1; p1.priority = 1;  // Menor
    p2.pid = 2; p2.priority = 5; // Maior
    p3.pid = 3; p3.priority = 3;  // Média
    
    scheduler.add(&p1);
    scheduler.add(&p2);
    scheduler.add(&p3);
    
    // Deve escolher por prioridade: P2 (5), P3 (3), P1 (1)
    assert(scheduler.fetchNext()->pid == 2);
    assert(scheduler.fetchNext()->pid == 3);
    assert(scheduler.fetchNext()->pid == 1);
    
    std::cout << "✓ Priority: Processos ordenados por prioridade\n";
}

void test_SJN() {
    std::cout << "\n=== TESTE: SJN - Menor Job Primeiro ===\n";
    
    Scheduler scheduler(SchedPolicy::SJN);
    
    PCB p1, p2, p3;
    p1.pid = 1; p1.burst_estimate = 100; // Maior
    p2.pid = 2; p2.burst_estimate = 50;  // Menor
    p3.pid = 3; p3.burst_estimate = 75;   // Médio
    
    scheduler.add(&p1);
    scheduler.add(&p2);
    scheduler.add(&p3);
    
    // Deve escolher por menor burst: P2 (50), P3 (75), P1 (100)
    assert(scheduler.fetchNext()->pid == 2);
    assert(scheduler.fetchNext()->pid == 3);
    assert(scheduler.fetchNext()->pid == 1);
    
    std::cout << "✓ SJN: Processos ordenados por menor burst\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES PRIORITÁRIOS: ESCALONADOR\n";
    std::cout << "========================================\n";
    
    try {
        test_FCFS();
        test_RoundRobin();
        test_Priority();
        test_SJN();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DO ESCALONADOR PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}

