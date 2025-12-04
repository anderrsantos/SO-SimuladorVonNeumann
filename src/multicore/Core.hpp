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

    CoreEvent(const CoreEvent&) = delete;
    CoreEvent& operator=(const CoreEvent&) = delete;

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

    // ============================
    //    NOVO → MÉTRICAS DO CORE
    // ============================
    void updateCoreTime();  // chamado pelo MultiCore a cada tick

    uint64_t getRunningTime() const { return time_running; }
    uint64_t getIdleTime() const { return time_idle; }
    uint64_t getWaitingIOTime() const { return time_waiting_io; }

    // ============================================
    //           NOVAS MÉTRICAS DO CORE
    // ============================================
    uint64_t time_running   = 0;
    uint64_t time_idle      = 0;
    uint64_t time_waiting_io = 0;

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

    std::vector<std::unique_ptr<IORequest>> ioRequests;

    int clockCounter;


};

