#include "Core.hpp"
#include <iostream>

// ==========================================================
//  CONSTRUTOR
// ==========================================================
Core::Core(int id, MemoryManager* memManager_, IOManager* ioManager_, bool* printLock)
    : coreId(id),
      memManager(memManager_),
      ioManager(ioManager_),
      current(nullptr),
      printLockPtr(printLock),
      printLockFlag(false),
      state(IDLE),
      counter(0),
      counterForEnd(5),
      endProgram(false),
      endExecution(false),
      clockCounter(0),
      time_running(0),
      time_idle(0),
      time_waiting_io(0)
{
    if (printLockPtr)
        printLockFlag = *printLockPtr;
}

Core::~Core() = default;


// ==========================================================
//     NOVO — MÉTRICAS DO CORE POR TICK
// ==========================================================
void Core::updateCoreTime()
{
    switch (state)
    {
        case RUNNING:
            time_running++;
            break;
        case WAITING_IO:
            time_waiting_io++;
            break;
        case IDLE:
        default:
            time_idle++;
            break;
    }
}


// ==========================================================
//  assignProcess
// ==========================================================
bool Core::assignProcess(PCB* pcb) {

    if (!pcb) return false;
    if (current != nullptr) return false;

    current = pcb;
    current->state = State::Running;

    // Reset pipeline counters
    counter = 0;
    counterForEnd = 5;
    endProgram = false;
    endExecution = false;
    clockCounter = 0;

    ioRequests.clear();
    uc.data.clear();   // <<< EVITA lixo no pipeline

    // Construção do ControlContext compatível com CONTROL_UNIT.hpp
    contextPtr = std::make_unique<ControlContext>(
        current->regBank,
        *memManager,
        ioRequests,
        printLockFlag,
        *current,
        counter,
        counterForEnd,
        endProgram,
        endExecution
    );

    state = RUNNING;
    return true;
}


// ==========================================================
//   stepOneCycle — executa 1 ciclo do pipeline
// ==========================================================
CoreEvent Core::stepOneCycle() {

    CoreEvent ev;
    ev.type = CoreEvent::NONE;
    ev.pcb = nullptr;
    ev.coreId = coreId;

    // Core sem processo
    if (!current || !contextPtr) {
        state = IDLE;
        return ev;
    }

    ControlContext& ctx = *contextPtr;

    // -------------------------------------------------------
    // GARANTIR TAMANHO DO PIPELINE
    // -------------------------------------------------------
    if (uc.data.size() <= static_cast<size_t>(ctx.counter))
        uc.data.resize(ctx.counter + 1);


    // =======================================================
    //                    PIPELINE
    // =======================================================

    // WB
    if (ctx.counter >= 4 && ctx.counterForEnd >= 1)
        uc.Write_Back(uc.data[ctx.counter - 4], ctx);

    // MEM
    if (ctx.counter >= 3 && ctx.counterForEnd >= 2)
        uc.Memory_Acess(uc.data[ctx.counter - 3], ctx);

    // EXEC
    if (ctx.counter >= 2 && ctx.counterForEnd >= 3)
        uc.Execute(uc.data[ctx.counter - 2], ctx);

    // DECODE
    if (ctx.counter >= 1 && ctx.counterForEnd >= 4) {
        current->stage_invocations.fetch_add(1);
        uc.Decode(ctx.registers, uc.data[ctx.counter - 1]);
    }

    // FETCH
    if (ctx.counter >= 0 && ctx.counterForEnd == 5) {
        Instruction_Data newData;
        uc.data[ctx.counter] = newData;
        uc.Fetch(ctx);
    }

    // Avança o tempo
    ctx.counter++;
    clockCounter++;
    current->pipeline_cycles.fetch_add(1);


    // =======================================================
    //     QUANTUM OU END
    // =======================================================
    if (clockCounter >= current->quantum || ctx.endProgram)
        ctx.endExecution = true;

    if (ctx.endExecution)
        ctx.counterForEnd--;


    // =======================================================
    //      FINALIZAÇÃO COMPLETA DO PIPELINE
    // =======================================================
    if (ctx.counterForEnd <= 0) {

        // -------------- PROCESSO FINALIZADO --------------
        if (ctx.endProgram) {
            current->state = State::Finished;

            ev.type = CoreEvent::FINISHED;
            ev.pcb = current;

            contextPtr.reset();
            state = IDLE;
            current = nullptr;

            return ev;
        }

        // -------------- PREEMPÇÃO POR QUANTUM --------------
        current->state = State::Ready;

        ev.type = CoreEvent::PREEMPTED;
        ev.pcb = current;

        contextPtr.reset();
        state = IDLE;
        current = nullptr;

        return ev;
    }


    // =======================================================
    //               BLOQUEIO POR I/O
    // =======================================================
    if (ctx.process.state == State::Blocked) {

        ev.type = CoreEvent::BLOCKED;
        ev.pcb = current;

        // Passa a requisição de IO para o evento
        ev.ioRequests = std::move(ioRequests);

        contextPtr.reset();
        state = WAITING_IO;
        current = nullptr;

        return ev;
    }


    // =======================================================
    //        CONTINUA EXECUTANDO — evento NONE
    // =======================================================
    return ev;
}

