/*
 * MetricsExtended.hpp
 * Extensão da classe Metrics para calcular métricas agregadas
 * necessárias para o relatório do trabalho
 */
#ifndef METRICS_EXTENDED_HPP
#define METRICS_EXTENDED_HPP

#include "Metrics.hpp"
#include "../multicore/Scheduler.hpp"
#include "../multicore/MultiCore.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <cmath>

class MetricsExtended {
public:
    // Estrutura para métricas agregadas por política
    struct PolicyMetrics {
        SchedPolicy policy;
        std::string policy_name;
        
        // Métricas básicas
        double avg_waiting_time;      // Tempo médio de espera
        double avg_turnaround_time;   // Tempo médio de retorno
        double cpu_utilization;       // Utilização média da CPU (%)
        double throughput;            // Processos completados / tempo total
        double efficiency;            // Eficiência (throughput / utilização)
        
        // Contadores
        size_t num_processes;
        uint64_t total_cycles;
        uint64_t total_service_time;
        uint64_t total_waiting_time;
        uint64_t total_turnaround_time;
    };
    
    // Estrutura para comparação Single-Core vs Multicore
    struct CoreComparison {
        size_t num_cores;
        double avg_waiting_time;
        double avg_turnaround_time;
        double cpu_utilization;
        double throughput;
        double speedup;  // Speedup global (multicore / single-core)
    };
    
    // Estrutura para métricas temporais (evolução ao longo do tempo)
    struct TemporalMetrics {
        uint64_t tick;
        double cpu_usage;        // % de CPU em uso neste tick
        double memory_usage;     // % de memória em uso neste tick
        double throughput_instant; // Throughput instantâneo
    };
    
    // Calcula métricas agregadas por política de escalonamento
    static PolicyMetrics calculatePolicyMetrics(
        const std::vector<Metrics::PCBReport>& reports,
        SchedPolicy policy,
        uint64_t total_cycles,
        size_t num_cores
    ) {
        PolicyMetrics pm;
        pm.policy = policy;
        
        // Nome da política
        switch (policy) {
            case SchedPolicy::FCFS: pm.policy_name = "FCFS"; break;
            case SchedPolicy::RR: pm.policy_name = "Round-Robin"; break;
            case SchedPolicy::PRIORITY: pm.policy_name = "Priority"; break;
            case SchedPolicy::SJN: pm.policy_name = "SJN"; break;
        }
        
        pm.num_processes = reports.size();
        pm.total_cycles = total_cycles;
        
        if (reports.empty()) {
            pm.avg_waiting_time = 0;
            pm.avg_turnaround_time = 0;
            pm.cpu_utilization = 0;
            pm.throughput = 0;
            pm.efficiency = 0;
            return pm;
        }
        
        // Calcular totais
        pm.total_waiting_time = 0;
        pm.total_turnaround_time = 0;
        pm.total_service_time = 0;
        
        for (const auto& r : reports) {
            pm.total_waiting_time += r.waiting;
            pm.total_turnaround_time += r.turnaround;
            pm.total_service_time += r.pipeline_cycles;
        }
        
        // Médias
        pm.avg_waiting_time = (double)pm.total_waiting_time / pm.num_processes;
        pm.avg_turnaround_time = (double)pm.total_turnaround_time / pm.num_processes;
        
        // Utilização da CPU = (tempo total de serviço) / (tempo total * num_cores)
        if (total_cycles > 0 && num_cores > 0) {
            pm.cpu_utilization = ((double)pm.total_service_time / (total_cycles * num_cores)) * 100.0;
            if (pm.cpu_utilization > 100.0) pm.cpu_utilization = 100.0;
        } else {
            pm.cpu_utilization = 0;
        }
        
        // Throughput = número de processos / tempo total
        if (total_cycles > 0) {
            pm.throughput = (double)pm.num_processes / total_cycles;
        } else {
            pm.throughput = 0;
        }
        
        // Eficiência = throughput / utilização (normalizada)
        if (pm.cpu_utilization > 0) {
            pm.efficiency = pm.throughput / (pm.cpu_utilization / 100.0);
        } else {
            pm.efficiency = 0;
        }
        
        return pm;
    }
    
    // Calcula comparação Single-Core vs Multicore
    static std::vector<CoreComparison> calculateCoreComparison(
        const std::vector<Metrics::PCBReport>& reports_single,
        const std::vector<Metrics::PCBReport>& reports_multi,
        uint64_t cycles_single,
        uint64_t cycles_multi,
        size_t num_cores
    ) {
        std::vector<CoreComparison> comparisons;
        
        // Single-Core
        CoreComparison single;
        single.num_cores = 1;
        if (!reports_single.empty()) {
            uint64_t total_wait = 0, total_turn = 0, total_service = 0;
            for (const auto& r : reports_single) {
                total_wait += r.waiting;
                total_turn += r.turnaround;
                total_service += r.pipeline_cycles;
            }
            single.avg_waiting_time = (double)total_wait / reports_single.size();
            single.avg_turnaround_time = (double)total_turn / reports_single.size();
            single.cpu_utilization = cycles_single > 0 ? 
                ((double)total_service / cycles_single) * 100.0 : 0;
            single.throughput = cycles_single > 0 ? 
                (double)reports_single.size() / cycles_single : 0;
        } else {
            single.avg_waiting_time = 0;
            single.avg_turnaround_time = 0;
            single.cpu_utilization = 0;
            single.throughput = 0;
        }
        single.speedup = 1.0; // Baseline
        comparisons.push_back(single);
        
        // Multicore
        CoreComparison multi;
        multi.num_cores = num_cores;
        if (!reports_multi.empty()) {
            uint64_t total_wait = 0, total_turn = 0, total_service = 0;
            for (const auto& r : reports_multi) {
                total_wait += r.waiting;
                total_turn += r.turnaround;
                total_service += r.pipeline_cycles;
            }
            multi.avg_waiting_time = (double)total_wait / reports_multi.size();
            multi.avg_turnaround_time = (double)total_turn / reports_multi.size();
            multi.cpu_utilization = cycles_multi > 0 && num_cores > 0 ? 
                ((double)total_service / (cycles_multi * num_cores)) * 100.0 : 0;
            multi.throughput = cycles_multi > 0 ? 
                (double)reports_multi.size() / cycles_multi : 0;
            
            // Speedup = throughput_multicore / throughput_singlecore
            if (single.throughput > 0) {
                multi.speedup = multi.throughput / single.throughput;
            } else {
                multi.speedup = 0;
            }
        } else {
            multi.avg_waiting_time = 0;
            multi.avg_turnaround_time = 0;
            multi.cpu_utilization = 0;
            multi.throughput = 0;
            multi.speedup = 0;
        }
        comparisons.push_back(multi);
        
        return comparisons;
    }
    
    // Salva métricas por política em CSV
    static void savePolicyMetricsCSV(
        const std::vector<PolicyMetrics>& metrics,
        const std::string& filename
    ) {
        std::ofstream fout(filename);
        fout << "policy,avg_waiting_time,avg_turnaround_time,cpu_utilization,"
             << "throughput,efficiency,num_processes,total_cycles\n";
        
        for (const auto& m : metrics) {
            fout << m.policy_name << ","
                 << std::fixed << std::setprecision(2) << m.avg_waiting_time << ","
                 << m.avg_turnaround_time << ","
                 << m.cpu_utilization << ","
                 << m.throughput << ","
                 << m.efficiency << ","
                 << m.num_processes << ","
                 << m.total_cycles << "\n";
        }
    }
    
    // Agrega múltiplas políticas em um único CSV de comparação
    static void saveAllPoliciesComparisonCSV(
        const std::vector<PolicyMetrics>& allMetrics,
        const std::string& filename
    ) {
        std::ofstream fout(filename);
        fout << "policy,avg_waiting_time,avg_turnaround_time,cpu_utilization,"
             << "throughput,efficiency,num_processes,total_cycles\n";
        
        for (const auto& m : allMetrics) {
            fout << m.policy_name << ","
                 << std::fixed << std::setprecision(2) << m.avg_waiting_time << ","
                 << m.avg_turnaround_time << ","
                 << m.cpu_utilization << ","
                 << m.throughput << ","
                 << m.efficiency << ","
                 << m.num_processes << ","
                 << m.total_cycles << "\n";
        }
    }
    
    // Salva comparação Single-Core vs Multicore em CSV
    static void saveCoreComparisonCSV(
        const std::vector<CoreComparison>& comparisons,
        const std::string& filename
    ) {
        std::ofstream fout(filename);
        fout << "num_cores,avg_waiting_time,avg_turnaround_time,"
             << "cpu_utilization,throughput,speedup\n";
        
        for (const auto& c : comparisons) {
            fout << c.num_cores << ","
                 << std::fixed << std::setprecision(2) << c.avg_waiting_time << ","
                 << c.avg_turnaround_time << ","
                 << c.cpu_utilization << ","
                 << c.throughput << ","
                 << c.speedup << "\n";
        }
    }
    
    // Imprime métricas por política no console
    static void printPolicyMetrics(const std::vector<PolicyMetrics>& metrics) {
        std::cout << "\n========== MÉTRICAS POR POLÍTICA DE ESCALONAMENTO ==========\n";
        for (const auto& m : metrics) {
            std::cout << "\nPolítica: " << m.policy_name << "\n";
            std::cout << "  Tempo médio de espera      : " 
                      << std::fixed << std::setprecision(2) << m.avg_waiting_time << " ciclos\n";
            std::cout << "  Tempo médio de retorno     : " 
                      << m.avg_turnaround_time << " ciclos\n";
            std::cout << "  Utilização média da CPU    : " 
                      << m.cpu_utilization << "%\n";
            std::cout << "  Throughput                 : " 
                      << m.throughput << " processos/ciclo\n";
            std::cout << "  Eficiência                 : " 
                      << m.efficiency << "\n";
            std::cout << "  Número de processos        : " << m.num_processes << "\n";
            std::cout << "  Total de ciclos            : " << m.total_cycles << "\n";
        }
        std::cout << "==========================================================\n";
    }
    
    // Imprime comparação Single-Core vs Multicore
    static void printCoreComparison(const std::vector<CoreComparison>& comparisons) {
        std::cout << "\n========== COMPARAÇÃO SINGLE-CORE vs MULTICORE ==========\n";
        for (const auto& c : comparisons) {
            std::cout << "\nCores: " << c.num_cores << "\n";
            std::cout << "  Espera média               : " 
                      << std::fixed << std::setprecision(2) << c.avg_waiting_time << " ciclos\n";
            std::cout << "  Retorno médio              : " 
                      << c.avg_turnaround_time << " ciclos\n";
            std::cout << "  Utilização                 : " 
                      << c.cpu_utilization << "%\n";
            std::cout << "  Throughput                 : " 
                      << c.throughput << " processos/ciclo\n";
            if (c.num_cores > 1) {
                std::cout << "  Speedup global            : " 
                          << c.speedup << "x\n";
            }
        }
        std::cout << "==========================================================\n";
    }
};

#endif // METRICS_EXTENDED_HPP

