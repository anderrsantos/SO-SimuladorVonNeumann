#pragma once
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <fstream>
#include <functional>
#include "../cpu/PCB.hpp"

// Estrutura gerada pelo CONTROL_UNIT.cpp
struct IORequest {
    std::string operation;       // "print", etc.
    std::string msg;             // texto / valor do IO
    PCB* process = nullptr;

    // Tempo de execução estimado
    std::chrono::milliseconds cost_cycles{100};
};

class IOManager {
public:
    IOManager();
    ~IOManager();

    // MULTICORE → usado pelo CoreEvent::BLOCKED
    void registerProcessWaitingForIO(
        PCB* pcb,
        std::vector<std::unique_ptr<IORequest>> requests,
        int latency_ms = 100
    );

    // API legada (sem requests)
    void registerProcessWaitingForIO(PCB* process);

    // Request avulsa
    void addRequest(std::unique_ptr<IORequest> request);

    // Step síncrono (modo antigo, ainda compatível)
    void step();

    // Número de processos em IO real
    size_t pendingCount() const;

    // Permite que o IOManager re-enfileire PCBs quando I/O termina
    void setReadyCallback(std::function<void(PCB*)> cb) {
        readyCallback = cb;
    }

private:

    // =========================
    // Estrutura interna da fila
    // =========================
    struct Entry {
        PCB* pcb;
        std::vector<std::unique_ptr<IORequest>> requests;
        int remaining_ms;
        std::chrono::steady_clock::time_point enqueue_time;
    };

    // =========================
    // FILA DE IO BLOQUEANTE
    // =========================
    std::vector<Entry> queue;
    mutable std::mutex queueLock;

    // Legacy
    std::vector<PCB*> waiting_processes;
    mutable std::mutex waiting_processes_lock;

    // =========================
    // ESTADO DOS DISPOSITIVOS
    // =========================
    bool printer_requesting;
    bool disk_requesting;
    bool network_requesting;
    std::mutex device_state_lock;

    // =========================
    // THREAD GERENTE DE IO
    // =========================
    std::thread managerThread;
    bool shutdown_flag = false;

    // Callback opcional → Scheduler.add(pcb)
    std::function<void(PCB*)> readyCallback = nullptr;

    // =========================
    // ARQUIVOS DE RESULTADO
    // =========================
    std::ofstream resultFile;
    std::ofstream outputFile;

    // =========================
    // FUNÇÕES INTERNAS
    // =========================
    void processEntry(Entry &e);
    void managerLoop();  // <--- O IO WORKER "DE VERDADE"
};
