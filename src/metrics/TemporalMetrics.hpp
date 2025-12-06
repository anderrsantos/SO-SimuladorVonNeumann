/*
 * TemporalMetrics.hpp
 * Coleta métricas temporais (evolução ao longo do tempo)
 * Para gráficos de uso de CPU, memória e throughput
 */
#ifndef TEMPORAL_METRICS_HPP
#define TEMPORAL_METRICS_HPP

#include <vector>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include "../multicore/MultiCore.hpp"
#include "../memory/MemoryManager.hpp"

struct TemporalSnapshot {
    uint64_t tick;
    double cpu_usage_percent;      // % de CPU em uso
    double memory_usage_percent;    // % de memória em uso
    double throughput_instant;      // Throughput instantâneo (processos completados / tick)
    size_t active_processes;       // Processos ativos neste momento
    size_t completed_processes;    // Processos completados até agora
};

class TemporalMetricsCollector {
private:
    std::vector<TemporalSnapshot> snapshots;
    size_t num_cores;
    size_t total_memory_size;
    uint64_t last_completed_count;
    uint64_t last_tick;
    
public:
    TemporalMetricsCollector(size_t cores, size_t mem_size)
        : num_cores(cores), total_memory_size(mem_size),
          last_completed_count(0), last_tick(0) {}
    
    // Coleta snapshot no tick atual
    void collectSnapshot(
        uint64_t tick,
        const MultiCore& multicore,
        const MemoryManager& memory,
        size_t completed_count
    ) {
        TemporalSnapshot snap;
        snap.tick = tick;
        
        // CPU Usage: quantos cores estão ativos
        size_t active_cores = multicore.countActiveCores();
        snap.cpu_usage_percent = num_cores > 0 ? 
            ((double)active_cores / num_cores) * 100.0 : 0.0;
        
        // Memory Usage: partições ocupadas
        const auto& partitions = memory.getPartitions();
        size_t occupied = 0;
        for (const auto& p : partitions) {
            if (!p.free) occupied++;
        }
        snap.memory_usage_percent = partitions.empty() ? 0.0 :
            ((double)occupied / partitions.size()) * 100.0;
        
        // Throughput instantâneo: processos completados desde último tick
        if (tick > last_tick) {
            uint64_t delta_completed = completed_count - last_completed_count;
            uint64_t delta_ticks = tick - last_tick;
            snap.throughput_instant = delta_ticks > 0 ? 
                (double)delta_completed / delta_ticks : 0.0;
        } else {
            snap.throughput_instant = 0.0;
        }
        
        snap.active_processes = active_cores;
        snap.completed_processes = completed_count;
        
        snapshots.push_back(snap);
        
        last_completed_count = completed_count;
        last_tick = tick;
    }
    
    // Salva em CSV para gráficos
    void saveCSV(const std::string& filename) const {
        std::ofstream fout(filename);
        fout << "tick,cpu_usage_percent,memory_usage_percent,"
             << "throughput_instant,active_processes,completed_processes\n";
        
        for (const auto& s : snapshots) {
            fout << s.tick << ","
                 << std::fixed << std::setprecision(2) << s.cpu_usage_percent << ","
                 << s.memory_usage_percent << ","
                 << s.throughput_instant << ","
                 << s.active_processes << ","
                 << s.completed_processes << "\n";
        }
    }
    
    // Limpa snapshots
    void clear() {
        snapshots.clear();
        last_completed_count = 0;
        last_tick = 0;
    }
    
    const std::vector<TemporalSnapshot>& getSnapshots() const {
        return snapshots;
    }
};

#endif // TEMPORAL_METRICS_HPP

