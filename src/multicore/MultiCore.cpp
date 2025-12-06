#include "MultiCore.hpp"
#include <iostream>

MultiCore::MultiCore(size_t n, MemoryManager* memMgr, IOManager* ioMgr, bool* printLock)
    : memManager(memMgr), ioManager(ioMgr), printLockPtr(printLock)
{
    cores.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        cores.emplace_back(std::make_unique<Core>(static_cast<int>(i), memManager, ioMgr, printLockPtr));
    }
}

MultiCore::~MultiCore() = default;

void MultiCore::assignReadyProcesses(const std::function<PCB*()>& fetchNext) {

    for (auto &core : cores) {

        // se o core está livre
        if (core->isIdle()) {

            // pega próximo processo pela política
            PCB* next = fetchNext();

            if (next != nullptr) {
                core->assignProcess(next);
            }
        }
    }
}

std::vector<CoreEvent> MultiCore::stepAll() {
    std::vector<CoreEvent> events;
    events.reserve(cores.size()); // evita realocações desnecessárias

    for (auto &cptr : cores) {
        CoreEvent ev = cptr->stepOneCycle();
        if (ev.type != CoreEvent::NONE) {
            // mover o evento para o vetor (CoreEvent contém unique_ptr -> não copiável)
            events.push_back(std::move(ev));
        }
    }
    return events;
}

bool MultiCore::hasActiveCores() const {
    for (auto &cptr : cores) if (!cptr->isIdle()) return true;
    return false;
}

size_t MultiCore::countActiveCores() const {
    size_t count = 0;
    for (auto &cptr : cores) {
        if (!cptr->isIdle()) count++;
    }
    return count;
}
