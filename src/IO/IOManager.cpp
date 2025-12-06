// IOManager.cpp
#include "IOManager.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std::chrono_literals;

IOManager::IOManager()
    : printer_requesting(false),
      disk_requesting(false),
      network_requesting(false),
      shutdown_flag(false)
{
    // abrir arquivos de resultado (append para não sobrescrever durante dev)
    resultFile.open("output/io_results.csv", std::ios::out | std::ios::app);
    outputFile.open("output/io_output.dat", std::ios::out | std::ios::app);

    // cabeçalho caso arquivo vazio
    if (resultFile.is_open()) {
        resultFile << "pid,op,msg,enqueued_ms,started_ms,completed_ms,wait_ms,service_ms\n";
    }

    // iniciar thread gerente
    managerThread = std::thread(&IOManager::managerLoop, this);
}

IOManager::~IOManager() {
    // sinalizar encerramento e aguardar a thread
    {
        std::lock_guard<std::mutex> lk(queueLock);
        shutdown_flag = true;
    }
    if (managerThread.joinable()) managerThread.join();

    if (resultFile.is_open()) resultFile.close();
    if (outputFile.is_open()) outputFile.close();
}

// API chamada pelo scheduler/core quando o processo bloqueia com requests
void IOManager::registerProcessWaitingForIO(
    PCB* pcb,
    std::vector<std::unique_ptr<IORequest>> requests,
    int latency_ms
) {
    if (!pcb) return;

    Entry e;
    e.pcb = pcb;
    e.requests = std::move(requests);
    e.enqueue_time = std::chrono::steady_clock::now();

    // Soma custos das requests para formar tempo estimado; inclui latency_ms
    int total_cost = latency_ms;
    for (const auto &r : e.requests) {
        if (r) total_cost += static_cast<int>(r->cost_cycles.count());
    }
    e.remaining_ms = std::max(1, total_cost);

    // marca processo bloqueado (se você quiser redundância; o Core já faz isso)
    pcb->state = State::Blocked;

    {
        std::lock_guard<std::mutex> lk(queueLock);
        queue.push_back(std::move(e));
    }
}

// API legada (sem requests)
void IOManager::registerProcessWaitingForIO(PCB* process) {
    if (!process) return;
    // cria Entry com request "nop" apenas para ocupar o dispositivo por 100ms
    auto req = std::make_unique<IORequest>();
    req->operation = "nop";
    req->msg = "";
    req->process = process;
    req->cost_cycles = std::chrono::milliseconds(100);

    std::vector<std::unique_ptr<IORequest>> v;
    v.push_back(std::move(req));
    registerProcessWaitingForIO(process, std::move(v), 0);
}

// adiciona request avulsa (processo já embutido no request)
void IOManager::addRequest(std::unique_ptr<IORequest> request) {
    if (!request) return;
    Entry e;
    e.pcb = request->process;
    e.enqueue_time = std::chrono::steady_clock::now();
    e.remaining_ms = static_cast<int>(request->cost_cycles.count());
    e.requests.push_back(std::move(request));

    if (e.pcb) e.pcb->state = State::Blocked;

    {
        std::lock_guard<std::mutex> lk(queueLock);
        queue.push_back(std::move(e));
    }
}

// compatibilidade: modo síncrono — processa decrementando remaining_ms com base em tempo passado
void IOManager::step() {
    auto now = std::chrono::steady_clock::now();
    // Para modo síncrono, usamos um delta fixo de 10ms por step
    const int delta_ms = 10;

    std::vector<Entry> finished;

    {
        std::lock_guard<std::mutex> lk(queueLock);
        for (auto &e : queue) {
            e.remaining_ms -= delta_ms;
            if (e.remaining_ms <= 0) finished.push_back(std::move(e));
        }
        // remover finished da queue
        queue.erase(std::remove_if(queue.begin(), queue.end(),
            [](const Entry &x){ return x.remaining_ms <= 0; }), queue.end());
    }

    // processar fora do lock
    for (auto &e : finished) {
        processEntry(e);
    }
}

size_t IOManager::pendingCount() const {
    std::lock_guard<std::mutex> lk(queueLock);
    return queue.size();
}

// função que realiza o trabalho quando uma Entry completa
void IOManager::processEntry(Entry &e) {
    if (!e.pcb) {
        // se não houver processo associado, apenas process requests (fire-and-forget)
        for (auto &rptr : e.requests) {
            if (!rptr) continue;
            // Apenas operações de saída (print) implementadas por enquanto
            if (rptr->operation == "print") {
                if (outputFile.is_open()) {
                    outputFile << "pid=0, print: " << rptr->msg << "\n";
                }
                std::cout << "[IO] print (no-pcb): " << rptr->msg << "\n";
            }
        }
        return;
    }

    auto start_time = std::chrono::steady_clock::now();
    // espera = tempo desde enqueue até começar a executar
    auto wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(start_time - e.enqueue_time).count();

    // executar cada request (simulado)
    int service_ms_total = 0;
    for (auto &rptr : e.requests) {
        if (!rptr) continue;
        int cost = static_cast<int>(rptr->cost_cycles.count());
        service_ms_total += cost;

        if (rptr->operation == "print") {
            // grava no outputFile e no console
            if (outputFile.is_open()) {
                outputFile << e.pcb->pid << ",PRINT," << rptr->msg << "\n";
            }
            std::cout << "[IO] (pid=" << e.pcb->pid << ") PRINT: " << rptr->msg << "\n";
        } else if (rptr->operation == "nop") {
            // nada
        } else {
            // operação desconhecida -> log
            std::cout << "[IO] (pid=" << e.pcb->pid << ") OP=" << rptr->operation
                      << " MSG=" << rptr->msg << "\n";
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto service_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Atualiza métricas no PCB (assume que pcb->io_cycles é std::atomic<uint64_t>)
    try {
        // io_cycles: total de ms gastos em I/O (espera + serviço)
        uint64_t total_ms = static_cast<uint64_t>(wait_ms + service_ms);
        e.pcb->io_cycles.fetch_add(total_ms);

        // Se PCB tiver campos adicionais para espera/serviço, ideal seria incrementá-los aqui.
        // Exemplo (comentado caso não existam):
        // e.pcb->io_wait_ms.fetch_add(static_cast<uint64_t>(wait_ms));
        // e.pcb->io_service_ms.fetch_add(static_cast<uint64_t>(service_ms));
    } catch (...) {
        // se o PCB não tiver os campos esperados, não queremos abortar;
        // apenas continuamos (não fatal).
    }

    // gravar no arquivo de métricas (resultFile)
    if (resultFile.is_open()) {
        for (auto &rptr : e.requests) {
            if (!rptr) continue;
            // enqueued_ms/start_ms/completed_ms calculados relativos a epoch
            auto enq = std::chrono::duration_cast<std::chrono::milliseconds>(e.enqueue_time.time_since_epoch()).count();
            auto st = std::chrono::duration_cast<std::chrono::milliseconds>(start_time.time_since_epoch()).count();
            auto ed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time.time_since_epoch()).count();

            resultFile << e.pcb->pid << ","
                       << rptr->operation << ","
                       << std::quoted(rptr->msg) << ","
                       << enq << "," << st << "," << ed << ","
                       << wait_ms << "," << service_ms << "\n";
        }
        resultFile.flush();
    }

    // Se houver callback (Scheduler.add), chamar para re-enfileirar o PCB como READY
    if (readyCallback) {
        // setar estado antes de callback
        e.pcb->state = State::Ready;
        readyCallback(e.pcb);
    } else {
        // sem callback — apenas marcar ready
        e.pcb->state = State::Ready;
    }
}

// Thread que gerencia a fila: subtrai tempo até 0 e finaliza entries
void IOManager::managerLoop() {
    auto last = std::chrono::steady_clock::now();
    while (true) {
        // checar shutdown
        {
            std::lock_guard<std::mutex> lk(queueLock);
            if (shutdown_flag && queue.empty()) break;
        }

        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        if (delta <= 0) delta = 1; // mínima progressão
        last = now;

        std::vector<Entry> completed;

        {
            std::lock_guard<std::mutex> lk(queueLock);

            for (auto &e : queue) {
                e.remaining_ms -= static_cast<int>(delta);
                if (e.remaining_ms <= 0) {
                    completed.push_back(std::move(e));
                }
            }

            // remover completadas da fila
            queue.erase(std::remove_if(queue.begin(), queue.end(),
                                       [](const Entry &x){ return x.remaining_ms <= 0; }),
                        queue.end());
        }

        // processar completadas sem segurar o lock
        for (auto &e : completed) {
            processEntry(e);
        }

        // dormir um pouco para não busy-wait (ajustável)
        std::this_thread::sleep_for(10ms);
    }
}
