#include "cache.hpp"
#include "../memory/MemoryManager.hpp"

#include <iostream>




// --------------------------------------------------
// Construtor
// --------------------------------------------------
Cache::Cache(size_t capacity_, CachePolicyType p)
    : capacity(capacity_), policy(p), cache_hits(0), cache_misses(0)
{
}

Cache::~Cache() {}

// --------------------------------------------------
// Escolhe qual entrada remover (FIFO ou LRU)
// --------------------------------------------------
void Cache::evictEntry(MemoryManager* memManager) {

    if (cacheMap.empty()) return;

    size_t addr_to_remove;

    // Seleção baseada em política
    if (policy == CachePolicyType::FIFO) {
        addr_to_remove = fifo_queue.front();
        fifo_queue.pop();
    } else { // LRU
        addr_to_remove = lru_list.front();
        lru_list.pop_front();
        lru_pos.erase(addr_to_remove);
    }

    auto it = cacheMap.find(addr_to_remove);
    if (it != cacheMap.end()) {

        // Write-back se sujo
        if (it->second.isDirty) {
            try {
                memManager->writeToFile(addr_to_remove, it->second.data);
            } catch (...) {
                std::cerr << "[Cache] ERRO: writeBack falhou em addr "
                          << addr_to_remove << std::endl;
            }
        }

        cacheMap.erase(it);
    }
}

// --------------------------------------------------
// GET
// --------------------------------------------------
size_t Cache::get(size_t address) {
    auto it = cacheMap.find(address);
    if (it == cacheMap.end()) {
        cache_misses++;
        return CACHE_MISS;
    }

    cache_hits++;

    // Atualização LRU
    if (policy == CachePolicyType::LRU) {
        auto pos = lru_pos[address];
        lru_list.erase(pos);
        lru_list.push_back(address);
        lru_pos[address] = std::prev(lru_list.end());
    }

    return it->second.data;
}

// --------------------------------------------------
// PUT
// --------------------------------------------------
void Cache::put(size_t address, size_t data, MemoryManager* memManager) {

    // Já existe → apenas atualiza (não conta reposição)
    if (cacheMap.find(address) != cacheMap.end()) {
        cacheMap[address].data = data;
        cacheMap[address].isValid = true;
        cacheMap[address].isDirty = false;

        if (policy == CachePolicyType::LRU) {
            auto pos = lru_pos[address];
            lru_list.erase(pos);
            lru_list.push_back(address);
            lru_pos[address] = std::prev(lru_list.end());
        }

        return;
    }

    // Cache cheia → remover conforme política
    if (cacheMap.size() >= capacity) {
        evictEntry(memManager);
    }

    // Nova entrada
    CacheEntry e;
    e.data = data;
    e.isValid = true;
    e.isDirty = false;

    cacheMap.emplace(address, e);

    // Inserir nas estruturas
    if (policy == CachePolicyType::FIFO) {
        fifo_queue.push(address);
    } else {
        lru_list.push_back(address);
        lru_pos[address] = std::prev(lru_list.end());
    }
}

// --------------------------------------------------
// UPDATE (write-back parcial)
// --------------------------------------------------
void Cache::update(size_t address, size_t data) {
    auto it = cacheMap.find(address);
    if (it == cacheMap.end()) {
        // no-write-allocate
        return;
    }

    it->second.data = data;
    it->second.isDirty = true;

    // Atualiza ordem LRU
    if (policy == CachePolicyType::LRU) {
        auto pos = lru_pos[address];
        lru_list.erase(pos);
        lru_list.push_back(address);
        lru_pos[address] = std::prev(lru_list.end());
    }
}

// --------------------------------------------------
// INVALIDAR TODA CACHE
// --------------------------------------------------
void Cache::invalidate() {
    fifo_queue = std::queue<size_t>();
    lru_list.clear();
    lru_pos.clear();
    cacheMap.clear();
}

// --------------------------------------------------
// Retorna linhas sujas
// --------------------------------------------------
std::vector<std::pair<size_t, size_t>> Cache::dirtyData() {
    std::vector<std::pair<size_t, size_t>> out;
    out.reserve(cacheMap.size());

    for (auto &kv : cacheMap) {
        if (kv.second.isDirty && kv.second.isValid) {
            out.emplace_back(kv.first, kv.second.data);
        }
    }

    return out;
}

// --------------------------------------------------
int Cache::get_hits()  { return cache_hits; }
int Cache::get_misses(){ return cache_misses; }
