#include "Scheduler.hpp"
#include <algorithm>

Scheduler::Scheduler(SchedPolicy p) : policy(p) {}

void Scheduler::add(PCB* pcb) {
    pcb->state = State::Ready;

    switch (policy) {
        case SchedPolicy::FCFS:
            readyQueueFCFS.push(pcb);
            break;

        case SchedPolicy::RR:
            readyQueueRR.push(pcb);
            break;

        case SchedPolicy::PRIORITY:
        case SchedPolicy::SJN:
            readyVector.push_back(pcb);
            break;
    }
}

PCB* Scheduler::fetchNext() {
    if (empty()) return nullptr;

    switch (policy) {

        case SchedPolicy::FCFS:
            if (!readyQueueFCFS.empty()) {
                PCB* p = readyQueueFCFS.front();
                readyQueueFCFS.pop();
                return p;
            }
            break;

        case SchedPolicy::RR:
            if (!readyQueueRR.empty()) {
                PCB* p = readyQueueRR.front();
                readyQueueRR.pop();
                return p;
            }
            break;

        case SchedPolicy::PRIORITY:
            if (!readyVector.empty()) {
                auto it = std::max_element(
                    readyVector.begin(), readyVector.end(),
                    [](PCB* a, PCB* b) {
                        return a->priority < b->priority;
                    }
                );
                PCB* chosen = *it;
                readyVector.erase(it);
                return chosen;
            }
            break;

        case SchedPolicy::SJN:
            if (!readyVector.empty()) {
                auto it = std::min_element(
                    readyVector.begin(), readyVector.end(),
                    [](PCB* a, PCB* b) {
                        return a->burst_estimate < b->burst_estimate;
                    }
                );
                PCB* chosen = *it;
                readyVector.erase(it);
                return chosen;
            }
            break;
    }

    return nullptr;
}

void Scheduler::unblock(PCB* pcb) {
    pcb->state = State::Ready;
    add(pcb);
}

bool Scheduler::empty() const {
    switch (policy) {
        case SchedPolicy::FCFS:
            return readyQueueFCFS.empty();
        case SchedPolicy::RR:
            return readyQueueRR.empty();
        case SchedPolicy::PRIORITY:
        case SchedPolicy::SJN:
            return readyVector.empty();
    }
    return true;
}
