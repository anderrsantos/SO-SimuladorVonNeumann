#ifndef CONTROL_UNIT_HPP
#define CONTROL_UNIT_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "REGISTER_BANK.hpp"
#include "PCB.hpp"
#include "../memory/MemoryManager.hpp"
#include "../IO/IOManager.hpp"
#include "../cpu/ULA.hpp"
#include "HASH_REGISTER.hpp"  // include correto conforme seu repositório

// =========================================================
//   Instruction_Data — estrutura usada em CONTROL_UNIT.cpp
// =========================================================
struct Instruction_Data {
    uint32_t rawInstruction = 0;    // instrução binária (32 bits)
    std::string op;                 // mnemônico identificado (ex: "ADD","LW",...)

    // registradores codificados como strings de bits ("01010")
    std::string source_register;    // rs
    std::string target_register;    // rt
    std::string destination_register;// rd (R-type)

    // endereço / immediate representado como string de bits
    // (para manter o traço e facilitar logging). Interpretar em bytes quando necessário.
    std::string addressRAMResult;

    // immediate já sign-extended (em 32 bits), usado nas operações I-type
    int32_t immediate = 0;
};


// =========================================================
//           ControlContext — usado pelo Core.cpp
// =========================================================
// Observação importante:
// - Neste design o PC é mantido em **bytes** (0,4,8,...).
// - MemoryManager internamente trabalha com índices de palavra (word index).
//   As conversões byte_address/4 são feitas no CONTROL_UNIT antes das leituras/escritas.
struct ControlContext {

    hw::REGISTER_BANK& registers;
    MemoryManager& memManager;
    std::vector<std::unique_ptr<IORequest>>& ioRequests;

    bool& printLock;

    PCB& process;

    // contadores usados pelo pipeline (windowing / flush on end)
    int& counter;
    int& counterForEnd;

    bool& endProgram;   // setado quando atingimos sentinel de fim
    bool& endExecution; // controla quando interromper (ex: bloqueio em I/O)

    ControlContext(
        hw::REGISTER_BANK& rb,
        MemoryManager& mm,
        std::vector<std::unique_ptr<IORequest>>& ioReqs,
        bool& pLock,
        PCB& proc,
        int& ctr,
        int& ctrEnd,
        bool& eProg,
        bool& eExec
    )
        : registers(rb),
          memManager(mm),
          ioRequests(ioReqs),
          printLock(pLock),
          process(proc),
          counter(ctr),
          counterForEnd(ctrEnd),
          endProgram(eProg),
          endExecution(eExec)
    {}

    ControlContext(const ControlContext&) = delete;
    ControlContext& operator=(const ControlContext&) = delete;
};


// =========================================================
//       Control_Unit — responsável pelo pipeline
// =========================================================
// Observações sobre endereçamento/PC:
// - PC no REGISTER_BANK deve conter endereço em **bytes**.
// - Antes de chamar MemoryManager::read/write converta: word_index = byte_address / 4.
// - Branches/J-type: calculam novo PC em bytes (J: instr26 << 2; branch: imm*4).
class Control_Unit {

public:

    Control_Unit();
    ~Control_Unit();

    // executa 1 ciclo do pipeline (implementação opcional no .cpp)
    void step(ControlContext& ctx);

    // estágios do pipeline
    void Fetch(ControlContext& context);
    void Decode(hw::REGISTER_BANK &registers, Instruction_Data &data);
    void Execute(Instruction_Data &data, ControlContext &context);
    void Execute_Immediate_Operation(hw::REGISTER_BANK &registers, Instruction_Data &data);
    void Execute_Aritmetic_Operation(hw::REGISTER_BANK &registers, Instruction_Data &data);

    void Execute_Operation(Instruction_Data &data, ControlContext &context);
    void Execute_Loop_Operation(hw::REGISTER_BANK &registers, Instruction_Data &data,
                                int &counter, int &counterForEnd, bool &programEnd,
                                MemoryManager &memManager, PCB &process);

    // Nota: "Memory_Acess" mantém o nome em português usado no .cpp (coerência)
    void Memory_Acess(Instruction_Data &data, ControlContext &context);
    void Write_Back(Instruction_Data &data, ControlContext &context);

    // decodificação / helpers
    std::string Identificacao_instrucao(uint32_t instruction, hw::REGISTER_BANK &registers);
    std::string Get_immediate(uint32_t instruction);
    std::string Get_destination_Register(uint32_t instruction);
    std::string Get_target_Register(uint32_t instruction);
    std::string Get_source_Register(uint32_t instruction);

    // debug opcional (log de operações em arquivo/console)
    void log_operation(const std::string &msg);

    // mapa global de instruções (mnemônico -> opcode_bin) — pode ser preenchido em runtime
    std::unordered_map<std::string, std::string> instructionMap;

    // mapper de registradores (hw::RegisterMapper) — usado para converter índices para nomes
    hw::RegisterMapper map;

    // buffer do pipeline (cada entrada contém a instrução decodificada para cada estágio)
    std::vector<Instruction_Data> data;
};

#endif // CONTROL_UNIT_HPP
