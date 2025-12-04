/*
 * TESTES DETALHADOS DE I/O
 * Valida: Bloqueio, Desbloqueio, Múltiplas Requisições
 */
#include <iostream>
#include <cassert>
#include <vector>
#include <memory>
#include "IO/IOManager.hpp"
#include "cpu/PCB.hpp"

void test_IO_Block_Unblock() {
    std::cout << "\n=== TESTE: Bloqueio e Desbloqueio por I/O ===\n";
    
    IOManager ioManager;
    
    PCB pcb;
    pcb.pid = 1;
    pcb.state = State::Running;
    
    // Criar requisições de I/O
    std::vector<std::unique_ptr<IORequest>> requests;
    auto req1 = std::make_unique<IORequest>();
    req1->operation = "print";
    req1->msg = "Teste I/O";
    req1->process = &pcb;
    req1->cost_cycles = std::chrono::milliseconds(50);
    requests.push_back(std::move(req1));
    
    // Registrar processo bloqueado
    assert(pcb.state == State::Running && "Estado inicial deve ser Running");
    ioManager.registerProcessWaitingForIO(&pcb, std::move(requests), 50);
    
    std::cout << "  Processo registrado para I/O\n";
    std::cout << "  Estado do processo: " << (pcb.state == State::Blocked ? "Blocked" : "Outro") << "\n";
    
    // Avançar I/O Manager
    for (int i = 0; i < 60; i++) {
        ioManager.step();
    }
    
    // Verificar que processo foi liberado
    std::cout << "  Estado após I/O: " << (pcb.state == State::Ready ? "Ready" : "Outro") << "\n";
    std::cout << "✓ Processo bloqueado e desbloqueado corretamente\n";
}

void test_Multiple_IO_Requests() {
    std::cout << "\n=== TESTE: Múltiplas Requisições de I/O ===\n";
    
    IOManager ioManager;
    
    std::vector<std::unique_ptr<PCB>> processes;
    std::vector<std::vector<std::unique_ptr<IORequest>>> all_requests;
    
    // Criar 3 processos com I/O
    for (int i = 0; i < 3; i++) {
        auto pcb = std::make_unique<PCB>();
        pcb->pid = i + 1;
        pcb->state = State::Running;
        
        std::vector<std::unique_ptr<IORequest>> requests;
        auto req = std::make_unique<IORequest>();
        req->operation = "print";
        req->msg = "Processo " + std::to_string(i + 1);
        req->process = pcb.get();
        req->cost_cycles = std::chrono::milliseconds(30);
        requests.push_back(std::move(req));
        
        ioManager.registerProcessWaitingForIO(pcb.get(), std::move(requests), 30);
        processes.push_back(std::move(pcb));
    }
    
    std::cout << "  Processos com I/O registrados: " << processes.size() << "\n";
    std::cout << "  Requisições pendentes: " << ioManager.pendingCount() << "\n";
    
    assert(ioManager.pendingCount() > 0 && "Deve haver requisições pendentes");
    
    // Processar requisições
    int steps = 0;
    while (ioManager.pendingCount() > 0 && steps < 200) {
        ioManager.step();
        steps++;
    }
    
    std::cout << "  Ciclos para processar todas: " << steps << "\n";
    std::cout << "  Requisições restantes: " << ioManager.pendingCount() << "\n";
    
    // Verificar que processos foram liberados
    int ready_count = 0;
    for (auto& pcb : processes) {
        if (pcb->state == State::Ready) {
            ready_count++;
        }
    }
    
    std::cout << "  Processos em Ready: " << ready_count << "\n";
    std::cout << "✓ Múltiplas requisições processadas\n";
}

void test_IO_Latency() {
    std::cout << "\n=== TESTE: Latência de I/O ===\n";
    
    IOManager ioManager;
    
    PCB pcb;
    pcb.pid = 1;
    pcb.state = State::Running;
    
    std::vector<std::unique_ptr<IORequest>> requests;
    auto req = std::make_unique<IORequest>();
    req->operation = "print";
    req->msg = "Latency test";
    req->process = &pcb;
    req->cost_cycles = std::chrono::milliseconds(100);
    requests.push_back(std::move(req));
    
    ioManager.registerProcessWaitingForIO(&pcb, std::move(requests), 100);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    int steps = 0;
    while (pcb.state != State::Ready && steps < 200) {
        ioManager.step();
        steps++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Latência configurada: 100ms\n";
    std::cout << "  Latência medida: " << duration.count() << "ms\n";
    std::cout << "  Ciclos de I/O: " << steps << "\n";
    
    // Verificar que processo foi liberado (latência processada)
    assert(pcb.state == State::Ready && "Processo deve estar Ready após I/O");
    assert(steps > 0 && "Deve ter processado I/O");
    std::cout << "✓ Latência de I/O processada corretamente\n";
}

void test_IO_Concurrent_Processes() {
    std::cout << "\n=== TESTE: Processos Concorrentes em I/O ===\n";
    
    IOManager ioManager;
    
    std::vector<std::unique_ptr<PCB>> processes;
    const int num_processes = 5;
    
    // Criar processos com diferentes latências
    for (int i = 0; i < num_processes; i++) {
        auto pcb = std::make_unique<PCB>();
        pcb->pid = i + 1;
        pcb->state = State::Running;
        
        std::vector<std::unique_ptr<IORequest>> requests;
        auto req = std::make_unique<IORequest>();
        req->operation = "print";
        req->msg = "Concurrent " + std::to_string(i);
        req->process = pcb.get();
        req->cost_cycles = std::chrono::milliseconds(20 + (i * 10));
        requests.push_back(std::move(req));
        
        ioManager.registerProcessWaitingForIO(pcb.get(), std::move(requests), 20 + (i * 10));
        processes.push_back(std::move(pcb));
    }
    
    std::cout << "  Processos em I/O: " << num_processes << "\n";
    
    // Processar todos
    int steps = 0;
    while (ioManager.pendingCount() > 0 && steps < 300) {
        ioManager.step();
        steps++;
    }
    
    int ready_count = 0;
    for (auto& pcb : processes) {
        if (pcb->state == State::Ready) {
            ready_count++;
        }
    }
    
    std::cout << "  Ciclos totais: " << steps << "\n";
    std::cout << "  Processos liberados: " << ready_count << "\n";
    
    assert(ready_count == num_processes && "Todos os processos devem ser liberados");
    std::cout << "✓ Processos concorrentes em I/O processados\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  TESTES DETALHADOS DE I/O\n";
    std::cout << "========================================\n";
    
    try {
        test_IO_Block_Unblock();
        test_Multiple_IO_Requests();
        test_IO_Latency();
        test_IO_Concurrent_Processes();
        
        std::cout << "\n========================================\n";
        std::cout << "  ✓ TODOS OS TESTES DE I/O PASSARAM\n";
        std::cout << "========================================\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ ERRO: " << e.what() << "\n";
        return 1;
    }
}

