#include "IOManager.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// ---------------------------------------------------
// CONSTRUTOR
// ---------------------------------------------------
IOManager::IOManager() :
    printer_requesting(false),
    disk_requesting(false),
    network_requesting(false),
    shutdown_flag(false)
{
    srand(static_cast<unsigned int>(time(nullptr)));

    resultFile.open("result.dat", std::ios::app);
    outputFile.open("output.dat", std::ios::app);

    if (!resultFile || !outputFile) {
        std::cerr << "Erro: não foi possível abrir arquivos de saída." << std::endl;
    }

    managerThread = std::thread(&IOManager::managerLoop, this);
}

IOManager::~IOManager() {
    shutdown_flag = true;
    if (managerThread.joinable())
        managerThread.join();
    resultFile.close();
    outputFile.close();
}

// ---------------------------------------------------
// OPÇÃO A — integração real com Core
// ---------------------------------------------------
void IOManager::registerProcessWaitingForIO(
    PCB* pcb,
    std::vector<std::unique_ptr<IORequest>> requests,
    int latency_ms
) {
    if (!pcb) return;

    std::lock_guard<std::mutex> lock(queueLock);
    Entry e;

    e.pcb = pcb;
    e.requests = std::move(requests);

    // calcula custo total/maior
    int max_ms = latency_ms;
    for (auto &r : e.requests) {
        if (r && r->cost_cycles.count() > max_ms)
            max_ms = r->cost_cycles.count();
    }

    e.remaining_ms = max_ms;

    pcb->state = State::Blocked;
    queue.push_back(std::move(e));
}

// ---------------------------------------------------
// Versão legada (mantida mas não usada)
// ---------------------------------------------------
void IOManager::registerProcessWaitingForIO(PCB* process) {
    if (!process) return;
    std::lock_guard<std::mutex> lock(waiting_processes_lock);
    waiting_processes.push_back(process);
}

// ---------------------------------------------------
void IOManager::addRequest(std::unique_ptr<IORequest> request) {
    if (!request || !request->process) return;

    std::lock_guard<std::mutex> lock(queueLock);
    Entry e;
    e.pcb = request->process;
    e.requests.push_back(std::move(request));
    e.remaining_ms = static_cast<int>(e.requests.back()->cost_cycles.count());
    e.pcb->state = State::Blocked;
    queue.push_back(std::move(e));
}

// ---------------------------------------------------
// step() — avança processamento
// ---------------------------------------------------
void IOManager::step() {
    std::vector<Entry> finished;

    {
        std::lock_guard<std::mutex> lock(queueLock);

        for (auto &e : queue) {
            int step_ms = 50;
            e.remaining_ms -= step_ms;
            e.pcb->io_cycles.fetch_add(step_ms);

            if (e.remaining_ms <= 0)
                finished.push_back(std::move(e));
        }

        // remover finalizados
        queue.erase(
            std::remove_if(
                queue.begin(), queue.end(),
                [&](const Entry &entry){
                    for (auto &done : finished)
                        if (done.pcb == entry.pcb) return true;
                    return false;
                }
            ),
            queue.end()
        );
    }

    // processar finalizados
    for (auto &e : finished) {

        // imprime/armazenar requests
        for (auto &r : e.requests) {
            if (r && !r->msg.empty()) {
                std::cout << "[IO] PID=" << e.pcb->pid << " => " << r->operation
                          << " | MSG: " << r->msg << "\n";

                if (resultFile)
                    resultFile << e.pcb->pid << " | " << r->operation << " | " << r->msg << "\n";

                if (outputFile)
                    outputFile << e.pcb->pid << "," << r->operation << "," 
                               << r->cost_cycles.count() << "\n";
            }
        }

        e.pcb->state = State::Ready;
    }
}

// ---------------------------------------------------
void IOManager::processEntry(Entry &e) {
    std::this_thread::sleep_for(std::chrono::milliseconds(e.remaining_ms));
    for (auto &r : e.requests)
        if (r && !r->msg.empty())
            std::cout << "[IO] PID=" << e.pcb->pid << " => " << r->msg << "\n";
    e.pcb->state = State::Ready;
}

// ---------------------------------------------------
void IOManager::managerLoop() {
    while (!shutdown_flag) {

        // (LEGACY — processa processos sem requests específicas)
        {
            std::lock_guard<std::mutex> lock(device_state_lock);
            if (rand() % 100 == 0) printer_requesting = true;
            if (rand() % 50 == 0)  disk_requesting = true;
        }

        {
            std::lock_guard<std::mutex> wlock(waiting_processes_lock);
            if (!waiting_processes.empty()) {

                std::lock_guard<std::mutex> dlock(device_state_lock);

                PCB* process_to_service = waiting_processes.front();
                std::unique_ptr<IORequest> new_request = nullptr;

                if (printer_requesting) {
                    new_request = std::make_unique<IORequest>();
                    new_request->operation = "print_job";
                    new_request->msg = "Legacy printing...";
                    new_request->process = process_to_service;
                    new_request->cost_cycles = std::chrono::milliseconds(200);
                    printer_requesting = false;
                }
                else if (disk_requesting) {
                    new_request = std::make_unique<IORequest>();
                    new_request->operation = "disk_read";
                    new_request->msg = "Legacy disk read...";
                    new_request->process = process_to_service;
                    new_request->cost_cycles = std::chrono::milliseconds(150);
                    disk_requesting = false;
                }

                if (new_request) {
                    waiting_processes.erase(waiting_processes.begin());
                    addRequest(std::move(new_request));
                }
            }
        }

        step();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

// ---------------------------------------------------
size_t IOManager::pendingCount() const {
    std::lock_guard<std::mutex> lock(queueLock);
    return queue.size();
}
