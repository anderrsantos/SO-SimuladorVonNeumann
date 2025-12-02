#ifndef PCB_HPP
#define PCB_HPP
/*
  PCB.hpp
  Versão estendida do PCB para o simulador multicore com partições fixas.
*/
#include <string>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "memory/cache.hpp"
#include "REGISTER_BANK.hpp" // necessidade de objeto completo dentro do PCB

// Estados possíveis do processo (compatível com CONTROL_UNIT)
enum class State {
    Ready,
    Running,
    Blocked,
    Finished
};

struct MemWeights {
    uint64_t cache = 1;      // custo por acesso à cache
    uint64_t primary = 5;    // custo por acesso à memória primária
    uint64_t secondary = 10; // custo por acesso à memória secundária
};

struct PCB {
    // Identificação
    int pid = 0;
    std::string name;

    // Escalonamento
    int quantum = 0;
    int priority = 0;
    uint64_t burst_estimate = 0; // para SJN (opcional / estimativa)

    // Estado
    State state = State::Ready;

    // Registradores / contexto de CPU
    hw::REGISTER_BANK regBank;

    // Partição / mapeamento lógico -> físico
    int partition_id = -1;        // índice da partição alocada (ou -1)
    uint32_t partition_base = 0;  // endereço físico base da partição
    uint32_t partition_size = 0;  // tamanho em bytes da partição

    // Offsets dentro da partição (tudo em palavras/endereços conforme seu MemoryManager)
    uint32_t data_bytes = 0;      // tamanho do segmento DATA (em palavras)
    uint32_t code_bytes = 0;      // tamanho do segmento CODE (em palavras)
    uint32_t initial_pc = 0;      // pc lógico inicial (normalmente = data_bytes)
    uint32_t job_length = 0;      // número de instruções

    // Armazenamento dos segmentos (populado pelo pcb_loader)
    std::vector<uint32_t> dataSegment; // valores iniciais do DATA (palavra por palavra)
    std::vector<uint32_t> codeSegment; // instruções (representadas como 32-bit cada)

    // Mapas auxiliares (labels / offsets) — preenchidos pelo parser quando disponível
    std::unordered_map<std::string, uint32_t> labelMap; // label -> instruction index
    std::unordered_map<std::string, uint32_t> dataMap;  // symbol -> offset dentro do DATA

    // Contadores de acesso à memória
    std::atomic<uint64_t> primary_mem_accesses{0};
    std::atomic<uint64_t> secondary_mem_accesses{0};
    std::atomic<uint64_t> memory_cycles{0};
    std::atomic<uint64_t> mem_accesses_total{0};
    std::atomic<uint64_t> extra_cycles{0};
    std::atomic<uint64_t> cache_mem_accesses{0};

    // Instrumentação detalhada
    std::atomic<uint64_t> pipeline_cycles{0};
    std::atomic<uint64_t> stage_invocations{0};
    std::atomic<uint64_t> mem_reads{0};
    std::atomic<uint64_t> mem_writes{0};

    // Contadores de cache
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};

    // IO
    std::atomic<uint64_t> io_cycles{0};

    // Pesos de memória (configuráveis por JSON)
    MemWeights memWeights;

    // Timestamps / métricas temporais (populados em runtime)
    uint64_t arrival_time = 0;
    uint64_t start_time = 0;
    uint64_t finish_time = 0;
    uint64_t wait_time = 0;
    uint64_t response_time = 0;

    // Construtor default ok
    PCB() = default;
};

// Contabilizar cache (usa atomics corretamente)
inline void contabiliza_cache(PCB &pcb, bool hit) {
    if (hit) {
        pcb.cache_hits.fetch_add(1);
    } else {
        pcb.cache_misses.fetch_add(1);
    }
}

#endif // PCB_HPP
