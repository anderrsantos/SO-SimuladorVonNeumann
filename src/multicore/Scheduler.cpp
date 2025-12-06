#include "Scheduler.hpp"
#include <iostream>

Scheduler::Scheduler(SchedPolicy p)
    : policy(p)
{}

void Scheduler::add(PCB* pcb) {
    if (!pcb) return;

    pcb->state = State::Ready;

    switch (policy) {

    case SchedPolicy::FCFS:
        readyQueueFCFS.push(pcb);
        break;

    case SchedPolicy::RR:
        readyQueueRR.push(pcb);
        break;

    case SchedPolicy::PRIORITY:
        readyVector.push_back(pcb);
        std::sort(readyVector.begin(), readyVector.end(),
                  [](PCB* a, PCB* b) { return a->priority > b->priority; });
        break;

    case SchedPolicy::SJN:
        readyVector.push_back(pcb);
        std::sort(readyVector.begin(), readyVector.end(),
                  [](PCB* a, PCB* b) { return a->job_length < b->job_length; });
        break;
    }
}

PCB* Scheduler::fetchNext() {

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
    case SchedPolicy::SJN:
        if (!readyVector.empty()) {
            PCB* p = readyVector.front();
            readyVector.erase(readyVector.begin());
            return p;
        }
        break;
    }

    return nullptr;
}

bool Scheduler::empty() const {
    switch (policy) {
    case SchedPolicy::FCFS: return readyQueueFCFS.empty();
    case SchedPolicy::RR:   return readyQueueRR.empty();
    case SchedPolicy::PRIORITY:
    case SchedPolicy::SJN:  return readyVector.empty();
    }
    return true;
}

void Scheduler::unblock(PCB* pcb) {
    if (!pcb) return;

    pcb->state = State::Ready;

    // Deve voltar exatamente como add()
    add(pcb);
}
