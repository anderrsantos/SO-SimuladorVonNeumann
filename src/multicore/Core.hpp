#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <atomic>

#include "../cpu/PCB.hpp"
#include "../IO/IOManager.hpp"
#include "../memory/MemoryManager.hpp"
#include "../cpu/CONTROL_UNIT.hpp"


// =====================================================================================
//                       CoreEvent — EVENTO DO PIPELINE
// =====================================================================================
// Contém: tipo do evento, PCB, id do core e IORequests movíveis.
// NÃO pode ser copiado porque contém unique_ptr.
// =====================================================================================

struct CoreEvent {

    enum Type {
        NONE = 0,
        FINISHED = 1,
        BLOCKED = 2,
        PREEMPTED = 3
    };

    Type type;
    PCB* pcb;
    int coreId;

    std::vector<std::unique_ptr<IORequest>> ioRequests;

    CoreEvent()
        : type(NONE), pcb(nullptr), coreId(-1) {}

    CoreEvent(Type t, PCB* p, int id)
        : type(t), pcb(p), coreId(id) {}

    // ❌ PROIBIR CÓPIA (required por causa do unique_ptr)
    CoreEvent(const CoreEvent&) = delete;
    CoreEvent& operator=(const CoreEvent&) = delete;

    // ✔ HABILITAR MOVIMENTO
    CoreEvent(CoreEvent&&) = default;
    CoreEvent& operator=(CoreEvent&&) = default;
};


// =====================================================================================
//                                      CORE
// =====================================================================================

class Core {

public:
    enum LocalState { IDLE = 0, RUNNING = 1, WAITING_IO = 2 };

    Core(int id,
         MemoryManager* memManager_,
         IOManager* ioManager_,
         bool* printLock);

    ~Core();

    bool assignProcess(PCB* pcb);
    CoreEvent stepOneCycle();

    bool isIdle() const { return current == nullptr; }
    int getId() const { return coreId; }
    LocalState getState() const { return state; }
    PCB* getCurrentPCB() const { return current; }

private:

    int coreId;

    MemoryManager* memManager;
    IOManager* ioManager;

    Control_Unit uc;

    std::unique_ptr<ControlContext> contextPtr;

    PCB* current;

    bool* printLockPtr;
    bool printLockFlag;

    LocalState state;

    int counter;
    int counterForEnd;
    bool endProgram;
    bool endExecution;

    // Buffer de IO Requests gerados pelo pipeline
    std::vector<std::unique_ptr<IORequest>> ioRequests;

    int clockCounter;
};
