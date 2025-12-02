#pragma once
#include <queue>
#include <vector>
#include <functional>
#include <memory>
#include "../cpu/PCB.hpp"

// Algoritmos suportados
enum class SchedPolicy {
    FCFS,
    PRIORITY,
    RR,
    SJN
};

class Scheduler {
private:
    SchedPolicy policy;

    // Filas principais
    std::queue<PCB*> readyQueueFCFS;
    std::queue<PCB*> readyQueueRR;
    std::vector<PCB*> readyVector;   // usado para PRIORITY e SJN

public:
    Scheduler(SchedPolicy p);

    void add(PCB* pcb);       // adiciona processo na fila READY
    PCB* fetchNext();         // retorna próximo processo para execução
    void unblock(PCB* pcb);   // processo volta de BLOCKED
    bool empty() const;

    // Auxiliares
    void setPolicy(SchedPolicy p) { policy = p; }
    SchedPolicy getPolicy() const { return policy; }
};
