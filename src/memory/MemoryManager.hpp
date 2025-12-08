#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <stdexcept>

#include "MAIN_MEMORY.hpp"
#include "SECONDARY_MEMORY.hpp"
#include "cache.hpp"
#include "../cpu/PCB.hpp"
#include "constants.hpp"

// -------------------------------------------------------------
//      Estrutura de Partições Fixas
// -------------------------------------------------------------
struct Partition {
    uint32_t base;      // endereço físico inicial
    uint32_t size;      // tamanho total
    int pid;            // processo dono (-1 = livre)
    bool free;

    Partition(uint32_t b, uint32_t s)
        : base(b), size(s), pid(-1), free(true) {}
};

// -------------------------------------------------------------
//                   MEMORY MANAGER
// -------------------------------------------------------------
class MemoryManager {
private:
    std::unique_ptr<MAIN_MEMORY> mainMemory;
    std::unique_ptr<SECONDARY_MEMORY> secondaryMemory;

public:
    std::unique_ptr<Cache> L1_cache;

private:
    uint32_t mainMemoryLimit;

    // Partições fixas
    std::vector<Partition> partitions;

public:
    MemoryManager(size_t mainMemorySize,
              size_t secondaryMemorySize,
              size_t cacheCapacity,
              CachePolicyType cachePolicy = CachePolicyType::FIFO);


    // ---------- Partições Fixas ----------
    void createPartitions(uint32_t partitionSize);
    Partition* allocateFixedPartition(PCB &pcb, uint32_t sizeRequired);
    void freePartition(int pid);

    uint32_t resolveAddress(uint32_t logicalAddr, const PCB &pcb);
    uint32_t readLogical(uint32_t logicalAddr, PCB &pcb);
    void writeLogical(uint32_t logicalAddr, uint32_t data, PCB &pcb);

    // ---------- Acesso Físico ----------
    uint32_t read(uint32_t address, PCB& process);
    void write(uint32_t address, uint32_t data, PCB& process);
    void writeToFile(uint32_t address, uint32_t data);

    // ---------- Auxiliar ----------
    inline void contabiliza_cache(PCB &pcb, bool hit) {
        if (hit) pcb.cache_hits.fetch_add(1);
        else     pcb.cache_misses.fetch_add(1);
    }

    const std::vector<Partition>& getPartitions() const { return partitions; }
};
