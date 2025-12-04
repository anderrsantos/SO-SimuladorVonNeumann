#pragma once
#include <queue>
#include <vector>
#include <algorithm>
#include "../cpu/PCB.hpp"

enum class SchedPolicy {
    FCFS,
    PRIORITY,
    RR,
    SJN
};

class Scheduler {
private:
    SchedPolicy policy;

    std::queue<PCB*> readyQueueFCFS;
    std::queue<PCB*> readyQueueRR;
    std::vector<PCB*> readyVector;

public:
    Scheduler(SchedPolicy p);

    void add(PCB* pcb);        // processo novo, volta de preempção, etc.
    PCB* fetchNext();          // entrega próximo PCB
    void unblock(PCB* pcb);    // AGORA ESSENCIAL para IO Worker!
    bool empty() const;

    void setPolicy(SchedPolicy p) { policy = p; }
    SchedPolicy getPolicy() const { return policy; }
};

