#include "MemoryManager.hpp"

// -------------------------------------------------------------
//                   CONSTRUTOR COMPLETO
// -------------------------------------------------------------
MemoryManager::MemoryManager(size_t mainMemorySize,
                             size_t secondaryMemorySize,
                             size_t cacheCapacity)
{
    mainMemory = std::make_unique<MAIN_MEMORY>(mainMemorySize);
    secondaryMemory = std::make_unique<SECONDARY_MEMORY>(secondaryMemorySize);

    // Cache com capacidade configurável
    L1_cache = std::make_unique<Cache>(cacheCapacity);

    mainMemoryLimit = mainMemorySize;
}

// -------------------------------------------------------------
//               CRIAÇÃO DE PARTIÇÕES FIXAS
// -------------------------------------------------------------
void MemoryManager::createPartitions(uint32_t partitionSize) {

    partitions.clear();
    uint32_t offset = 0;

    while (offset + partitionSize <= mainMemoryLimit) {
        partitions.emplace_back(offset, partitionSize);
        offset += partitionSize;
    }
}

// -------------------------------------------------------------
//                ALOCAÇÃO DE PARTIÇÃO FIXA
// -------------------------------------------------------------
Partition* MemoryManager::allocateFixedPartition(PCB &pcb, uint32_t sizeRequired) {

    for (auto &p : partitions) {
        if (p.free && p.size >= sizeRequired) {

            p.free = false;
            p.pid = pcb.pid;

            pcb.partition_id   = &p - &partitions[0];  // id
            pcb.partition_base = p.base;
            pcb.partition_size = p.size;

            return &p;
        }
    }
    return nullptr;
}

// -------------------------------------------------------------
void MemoryManager::freePartition(int pid) {
    for (auto &p : partitions) {
        if (p.pid == pid) {
            p.free = true;
            p.pid  = -1;
        }
    }
}

// -------------------------------------------------------------
//         ENDEREÇO LÓGICO → FÍSICO (partições)
// -------------------------------------------------------------
uint32_t MemoryManager::resolveAddress(uint32_t logicalAddr, const PCB &pcb) {

    for (const auto &p : partitions) {
        if (p.pid == pcb.pid) {

            if (logicalAddr >= p.size)
                throw std::out_of_range("Logical Addr > Partition Size");

            return p.base + logicalAddr;
        }
    }
    throw std::runtime_error("Process does not own a partition");
}

// -------------------------------------------------------------
//                   LEITURA LÓGICA
// -------------------------------------------------------------
uint32_t MemoryManager::readLogical(uint32_t logicalAddr, PCB &pcb) {
    uint32_t phys = resolveAddress(logicalAddr, pcb);
    return read(phys, pcb);
}

// -------------------------------------------------------------
//                   ESCRITA LÓGICA
// -------------------------------------------------------------
void MemoryManager::writeLogical(uint32_t logicalAddr,
                                 uint32_t data,
                                 PCB &pcb)
{
    uint32_t phys = resolveAddress(logicalAddr, pcb);
    write(phys, data, pcb);
}

// -------------------------------------------------------------
//                     LEITURA FÍSICA
// -------------------------------------------------------------
uint32_t MemoryManager::read(uint32_t address, PCB& process) {

    process.mem_accesses_total.fetch_add(1);
    process.mem_reads.fetch_add(1);

    // Se não houver cache configurada, ler diretamente da memória
    if (!L1_cache) {
        if (address < mainMemoryLimit) {
            process.primary_mem_accesses.fetch_add(1);
            process.memory_cycles.fetch_add(process.memWeights.primary);
            return mainMemory->ReadMem(address);
        } else {
            process.secondary_mem_accesses.fetch_add(1);
            process.memory_cycles.fetch_add(process.memWeights.secondary);
            uint32_t secAddr = address - mainMemoryLimit;
            return secondaryMemory->ReadMem(secAddr);
        }
    }

    // Tenta pegar da cache
    size_t cache_data = L1_cache->get(address);
    if (cache_data != CACHE_MISS) {

        process.cache_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.cache);

        contabiliza_cache(process, true);
        return static_cast<uint32_t>(cache_data);
    }

    contabiliza_cache(process, false);

    // MISS → ler RAM ou secundária
    uint32_t data_from_mem;

    if (address < mainMemoryLimit) {
        process.primary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.primary);

        data_from_mem = mainMemory->ReadMem(address);
    } else {
        process.secondary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.secondary);

        uint32_t secAddr = address - mainMemoryLimit;
        data_from_mem = secondaryMemory->ReadMem(secAddr);
    }

    // Coloca em cache (write-allocate on read miss)
    L1_cache->put(address, data_from_mem, this);

    return data_from_mem;
}

// -------------------------------------------------------------
//                   ESCRITA FÍSICA
// -------------------------------------------------------------
void MemoryManager::write(uint32_t address, uint32_t data, PCB& process) {

    process.mem_accesses_total.fetch_add(1);
    process.mem_writes.fetch_add(1);

    // 1) Escreve imediatamente na memória principal/secundária (write-through)
    writeToFile(address, data);

    // 2) Agora atualiza a cache (se houver)
    if (!L1_cache) {
        // Sem cache: contadores de memória já atualizados por writeToFile
        process.memory_cycles.fetch_add(process.memWeights.primary); // aproximação
        return;
    }

    // Verifica se a linha está na cache
    size_t cache_data = L1_cache->get(address);

    if (cache_data == CACHE_MISS) {
        // MISS: contabiliza miss e aloca/insere a linha com o novo valor
        contabiliza_cache(process, false);
        // Inserir o novo dado na cache (write-allocate)
        L1_cache->put(address, data, this);
    } else {
        // HIT: contabiliza hit e atualiza a entrada
        contabiliza_cache(process, true);
        L1_cache->update(address, data);
    }

    // estatísticas de cache/memória
    process.cache_mem_accesses.fetch_add(1);
    // já contabilizamos cycles pela memória (writeToFile), ajustar ciclos de cache também
    process.memory_cycles.fetch_add(process.memWeights.cache);
}

// -------------------------------------------------------------
//               WRITE-BACK da MEMÓRIA
// -------------------------------------------------------------
void MemoryManager::writeToFile(uint32_t address, uint32_t data) {

    if (address < mainMemoryLimit) {
        mainMemory->WriteMem(address, data);
    } else {
        uint32_t secondaryAddress = address - mainMemoryLimit;
        secondaryMemory->WriteMem(secondaryAddress, data);
    }
}
