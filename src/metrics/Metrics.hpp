#ifndef METRICS_HPP
#define METRICS_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "../cpu/PCB.hpp"

class Metrics {

public:

    struct PCBReport {
        uint32_t pid;
        std::string name;

        uint64_t arrival;
        uint64_t start;
        uint64_t finish;

        uint64_t turnaround;
        uint64_t waiting;
        uint64_t response;
        uint64_t pipeline_cycles;

        // memória
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t mem_accesses;
        uint64_t io_cycles;
    };

    // Coleta dados de todos os pcbs
    static std::vector<PCBReport> collect(const std::vector<std::unique_ptr<PCB>>& allPCBs) {
        std::vector<PCBReport> reports;

        for (auto& up : allPCBs) {
            PCB* p = up.get();

            PCBReport r;
            r.pid = p->pid;
            r.name = p->name;

            r.arrival = p->arrival_time;
            r.start   = p->start_time;
            r.finish  = p->finish_time;

            r.turnaround = p->finish_time  - p->arrival_time;
            r.response   = p->response_time;
            r.pipeline_cycles = p->pipeline_cycles.load();

            uint64_t service = p->pipeline_cycles.load();
            uint64_t total   = r.turnaround;
            r.waiting = (total > service) ? (total - service) : 0;

            // Memória
            r.cache_hits   = p->cache_hits.load();
            r.cache_misses = p->cache_misses.load();
            r.mem_accesses = p->mem_accesses_total.load();
            r.io_cycles    = p->io_cycles.load();

            reports.push_back(r);
        }
        return reports;
    }

    // Imprime na tela em formato bonito
    static void printConsole(const std::vector<PCBReport>& R) {
        std::cout << "\n================ MÉTRICAS ==================\n";
        for (auto& r : R) {
            std::cout << "PID " << r.pid << " (" << r.name << ")\n";
            std::cout << "  Arrival      : " << r.arrival << "\n";
            std::cout << "  Start        : " << r.start << "\n";
            std::cout << "  Finish       : " << r.finish << "\n";
            std::cout << "  Turnaround   : " << r.turnaround << "\n";
            std::cout << "  Waiting      : " << r.waiting << "\n";
            std::cout << "  Response     : " << r.response << "\n";
            std::cout << "  Pipeline cyc.: " << r.pipeline_cycles << "\n";
            std::cout << "  Cache hits   : " << r.cache_hits << "\n";
            std::cout << "  Cache misses : " << r.cache_misses << "\n";
            std::cout << "  Mem access   : " << r.mem_accesses << "\n";
            std::cout << "  IO cycles    : " << r.io_cycles << "\n";
            std::cout << "---------------------------------------------\n";
        }
    }

    // Exporta CSV
    static void saveCSV(const std::vector<PCBReport>& R, const std::string& file) {
        std::ofstream fout(file);
        fout << "pid,name,arrival,start,finish,turnaround,waiting,response,"
                "pipeline,cache_hits,cache_misses,mem_accesses,io_cycles\n";

        for (auto& r : R) {
            fout   << r.pid << "," << r.name << ","
                   << r.arrival << "," << r.start << "," << r.finish << ","
                   << r.turnaround << "," << r.waiting << "," << r.response << ","
                   << r.pipeline_cycles << ","
                   << r.cache_hits << "," << r.cache_misses << ","
                   << r.mem_accesses << "," << r.io_cycles
                   << "\n";
        }
    }

    // Exporta JSON
    static void saveJSON(const std::vector<PCBReport>& R, const std::string& file) {
        std::ofstream f(file);
        f << "[\n";
        for (size_t i = 0; i < R.size(); i++) {
            auto& r = R[i];
            f << "  {\n";
            f << "    \"pid\": " << r.pid << ",\n";
            f << "    \"name\": \"" << r.name << "\",\n";
            f << "    \"arrival\": " << r.arrival << ",\n";
            f << "    \"start\": " << r.start << ",\n";
            f << "    \"finish\": " << r.finish << ",\n";
            f << "    \"turnaround\": " << r.turnaround << ",\n";
            f << "    \"waiting\": " << r.waiting << ",\n";
            f << "    \"response\": " << r.response << ",\n";
            f << "    \"pipeline\": " << r.pipeline_cycles << ",\n";
            f << "    \"cache_hits\": " << r.cache_hits << ",\n";
            f << "    \"cache_misses\": " << r.cache_misses << ",\n";
            f << "    \"mem_accesses\": " << r.mem_accesses << ",\n";
            f << "    \"io_cycles\": " << r.io_cycles << "\n";
            f << "  }";
            if (i != R.size() - 1) f << ",";
            f << "\n";
        }
        f << "]\n";
    }

};

#endif
