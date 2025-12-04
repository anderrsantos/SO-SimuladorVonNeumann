#ifndef METRICS_HPP
#define METRICS_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "../cpu/PCB.hpp"
#include "../multicore/Core.hpp"

class Metrics {

public:

    // ============================================================
    //                 RELATÓRIO DE PROCESSOS (PCB)
    // ============================================================
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

    // ============================================================
    //                   RELATÓRIO DE CORES
    // ============================================================
    struct CoreReport {
        int coreId = -1;

        uint64_t running_time = 0;
        uint64_t waiting_io_time = 0;
        uint64_t idle_time = 0;
    };

    // ============================================================
    //      COLETA DE MÉTRICAS DOS PROCESSOS (PCB)
    // ============================================================
    static std::vector<PCBReport> collect(const std::vector<std::unique_ptr<PCB>>& allPCBs) {
        std::vector<PCBReport> reports;

        for (auto& up : allPCBs) {
            PCB* p = up.get();

            PCBReport r{};
            r.pid = p->pid;
            r.name = p->name;

            r.arrival = p->arrival_time;
            r.start   = p->start_time;
            r.finish  = p->finish_time;

            r.turnaround = p->finish_time - p->arrival_time;
            r.response   = p->response_time;
            r.pipeline_cycles = p->pipeline_cycles.load();

            uint64_t service = r.pipeline_cycles;
            r.waiting = (r.turnaround > service) ? (r.turnaround - service) : 0;

            r.cache_hits   = p->cache_hits.load();
            r.cache_misses = p->cache_misses.load();
            r.mem_accesses = p->mem_accesses_total.load();
            r.io_cycles    = p->io_cycles.load();

            reports.push_back(r);
        }

        return reports;
    }

    // ============================================================
    //         COLETA DE MÉTRICAS DOS CORES
    // ============================================================
    static std::vector<CoreReport> collectCores(const std::vector<std::unique_ptr<Core>>& cores)
    {
        std::vector<CoreReport> R;
        R.reserve(cores.size());

        for (auto& cptr : cores) {
            Core* c = cptr.get();
            CoreReport r;

            r.coreId = c->getId();
            r.running_time     = c->time_running;
            r.waiting_io_time  = c->time_waiting_io;
            r.idle_time        = c->time_idle;

            R.push_back(r);
        }

        return R;
    }

    // ============================================================
    //                 PRINT MÉTRICAS PCB
    // ============================================================
    static void printConsole(const std::vector<PCBReport>& R) {
        std::cout << "\n================ MÉTRICAS (PROCESSOS) ==================\n";
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
            std::cout << "--------------------------------------------------------\n";
        }
    }

    // ============================================================
    //                 PRINT MÉTRICAS CORE
    // ============================================================
    static void printCoreMetrics(const std::vector<CoreReport>& R) {
        std::cout << "\n================ MÉTRICAS (CORES) ==================\n";

        for (auto& c : R) {
            std::cout << "CORE " << c.coreId << "\n";
            std::cout << "  Tempo executando      : " << c.running_time << "\n";
            std::cout << "  Tempo esperando I/O   : " << c.waiting_io_time << "\n";
            std::cout << "  Tempo ocioso          : " << c.idle_time << "\n";
            std::cout << "-----------------------------------------------------\n";
        }
    }

    // ============================================================
    //                   SALVAR CSV
    // ============================================================
    static void saveCoreCSV(const std::vector<CoreReport>& R, const std::string& file)
    {
        std::ofstream f(file);
        f << "core_id,running,waiting_io,idle\n";

        for (auto& c : R) {
            f << c.coreId << ","
              << c.running_time << ","
              << c.waiting_io_time << ","
              << c.idle_time << "\n";
        }
    }
};

#endif
