/*
 * SCRIPT PRINCIPAL: Executa todos os testes prioritários
 * Compilar: g++ -std=c++17 -I../ run_all_priority_tests.cpp -o run_tests
 * Executar: ./run_tests
 */
#include <iostream>
#include <cstdlib>
#include <string>

int main() {
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║   EXECUTANDO TESTES PRIORITÁRIOS DO SIMULADOR    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    
    // Lista de testes para executar
    std::vector<std::string> tests = {
        "test_scheduler_priority",
        "test_memory_critical",
        "test_pipeline_basic",
        "test_integration_complete",
        "test_edge_cases"
    };
    
    for (const auto& test : tests) {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "Executando: " << test << "\n";
        std::cout << std::string(50, '=') << "\n";
        
        std::string command = "./" + test;
        int result = system(command.c_str());
        
        total_tests++;
        if (result == 0) {
            passed_tests++;
            std::cout << "✓ " << test << " PASSOU\n";
        } else {
            failed_tests++;
            std::cout << "✗ " << test << " FALHOU (código: " << result << ")\n";
        }
    }
    
    // Resumo final
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "RESUMO DOS TESTES\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "Total de testes: " << total_tests << "\n";
    std::cout << "✓ Passaram: " << passed_tests << "\n";
    std::cout << "✗ Falharam: " << failed_tests << "\n";
    
    if (failed_tests == 0) {
        std::cout << "\n🎉 TODOS OS TESTES PASSARAM!\n";
        return 0;
    } else {
        std::cout << "\n⚠️  ALGUNS TESTES FALHARAM\n";
        return 1;
    }
}

