#pragma once
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <fstream>
#include "../cpu/PCB.hpp"

// Estrutura gerada pelo CONTROL_UNIT.cpp
struct IORequest {
    std::string operation;       // "print", "read", "write", etc.
    std::string msg;             // texto/valor do IO
    PCB* process = nullptr;
    std::chrono::milliseconds cost_cycles{100}; // tempo padrão de IO
};

class IOManager {
public:
    IOManager();
    ~IOManager();

    // OPÇÃO A — integração completa com o Core:
    // move os IORequests que vieram do pipeline do Core
    void registerProcessWaitingForIO(
        PCB* pcb,
        std::vector<std::unique_ptr<IORequest>> requests,
        int latency_ms = 100
    );

    // API legada — não usada no multicore, mas deixada por compatibilidade
    void registerProcessWaitingForIO(PCB* process);

    // Adiciona request avulsa
    void addRequest(std::unique_ptr<IORequest> request);

    // Usado pela thread interna
    void step();

    size_t pendingCount() const;

private:
    struct Entry {
        PCB* pcb;
        std::vector<std::unique_ptr<IORequest>> requests;
        int remaining_ms;
    };

    std::vector<Entry> queue;    // processos em IO real
    mutable std::mutex queueLock;

    std::vector<PCB*> waiting_processes; // legacy
    mutable std::mutex waiting_processes_lock;

    bool printer_requesting;
    bool disk_requesting;
    bool network_requesting;
    std::mutex device_state_lock;

    std::ofstream resultFile;
    std::ofstream outputFile;

    std::thread managerThread;
    bool shutdown_flag;

    void processEntry(Entry &e);
    void managerLoop();
};
