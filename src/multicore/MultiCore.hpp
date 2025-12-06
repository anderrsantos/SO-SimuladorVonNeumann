#pragma once
#include <vector>
#include <functional>
#include <memory>
#include "Core.hpp"
#include "../cpu/PCB.hpp"
#include "../memory/MemoryManager.hpp"
#include "../IO/IOManager.hpp"

// MultiCore manager: contém N cores, delega assign e stepAll
class MultiCore {
public:
    MultiCore(size_t n, MemoryManager* memMgr, IOManager* ioMgr, bool* printLock = nullptr);
    ~MultiCore();

    // assignReadyProcesses: fetchNext must return next ready PCB* (or nullptr if none)
    // It will be called repeatedly until all free cores are filled.
    void assignReadyProcesses(const std::function<PCB*()>& fetchNext);

    // stepAll: avança 1 ciclo em todos os cores. Retorna lista de events (finished/blocked/preempted)
    std::vector<CoreEvent> stepAll();

    bool hasActiveCores() const;
    size_t numCores() const { return cores.size(); }
    size_t countActiveCores() const; // Conta quantos cores estão ativos

    // ADICIONAR AQUI:
    const std::vector<std::unique_ptr<Core>>& getCores() const {
        return cores;
    }

private:
    std::vector<std::unique_ptr<Core>> cores;
    MemoryManager* memManager;
    IOManager* ioManager;
    bool* printLockPtr;
};
