// MultiCore.cpp
#include "MultiCore.hpp"
#include <iostream>

MultiCore::MultiCore(size_t n, MemoryManager* memMgr, IOManager* ioMgr, bool* printLock)
    : memManager(memMgr), ioManager(ioMgr), printLockPtr(printLock)
{
    cores.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        cores.emplace_back(std::make_unique<Core>(static_cast<int>(i), memMgr, ioMgr, printLockPtr));
    }
}

MultiCore::~MultiCore() = default;

void MultiCore::assignReadyProcesses(const std::function<PCB*()>& fetchNext) {
    for (auto &cptr : cores) {
        if (!cptr) continue;

        if (cptr->isIdle()) {
            PCB* p = fetchNext();
            if (p == nullptr) continue;

            bool ok = cptr->assignProcess(p);
            if (!ok) {
                std::cerr << "[MultiCore] Warning: failed to assign PCB pid=" << p->pid
                          << " to core " << cptr->getId() << "\n";
            }
        }
    }
}

std::vector<CoreEvent> MultiCore::stepAll() {
    std::vector<CoreEvent> events;
    events.reserve(cores.size());

    for (auto &cptr : cores) {
        if (!cptr) continue;

        // 🔥 NOVO: mede tempo deste core no tick atual
        cptr->updateCoreTime();

        CoreEvent ev = cptr->stepOneCycle();

        if (ev.coreId < 0) ev.coreId = cptr->getId();

        if (ev.type == CoreEvent::BLOCKED) {
            if (ioManager) {
                try {
                    ioManager->registerProcessWaitingForIO(ev.pcb, std::move(ev.ioRequests), 100);
                } catch (const std::exception &ex) {
                    std::cerr << "[MultiCore] Exception while registering IO: " << ex.what() << "\n";
                } catch (...) {
                    std::cerr << "[MultiCore] Unknown exception while registering IO\n";
                }
            }

            ev.ioRequests.clear();
            events.push_back(std::move(ev));
            continue;
        }

        events.push_back(std::move(ev));
    }

    return events;
}

bool MultiCore::hasActiveCores() const {
    for (const auto &cptr : cores) {
        if (!cptr) continue;
        if (!cptr->isIdle()) return true;
    }
    return false;
}

size_t MultiCore::countActiveCores() const {
    size_t count = 0;
    for (auto &cptr : cores) {
        if (!cptr->isIdle()) count++;
    }
    return count;
}
