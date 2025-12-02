#include "cache.hpp"
#include "../memory/MemoryManager.hpp"
#include <iostream>

#ifndef CACHE_MISS
#define CACHE_MISS SIZE_MAX
#endif

// --------------------------------------------------
// Construtor com capacidade configurável
// --------------------------------------------------
Cache::Cache(size_t capacity_)
    : capacity(capacity_),
      cache_hits(0),
      cache_misses(0)
{
}

Cache::~Cache() {}

// --------------------------------------------------
size_t Cache::get(size_t address) {
    auto it = cacheMap.find(address);
    if (it == cacheMap.end()) {
        cache_misses++;
        return CACHE_MISS;
    }
    cache_hits++;
    return it->second.data;
}

// --------------------------------------------------
void Cache::put(size_t address, size_t data, MemoryManager* memManager) {

    // Se já existe, apenas atualiza (não reinserimos na FIFO)
    if (cacheMap.find(address) != cacheMap.end()) {
        cacheMap[address].data = data;
        return;
    }

    // Evict se cheia
    if (cacheMap.size() >= capacity) {
        if (!fifo_queue.empty()) {
            size_t addr_to_remove = fifo_queue.front();
            fifo_queue.pop();

            auto it = cacheMap.find(addr_to_remove);
            if (it != cacheMap.end()) {

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
    }

    // Inserir nova entrada
    CacheEntry e;
    e.data = data;
    e.isValid = true;
    e.isDirty = false;

    cacheMap.emplace(address, e);
    fifo_queue.push(address);
}

// --------------------------------------------------
void Cache::update(size_t address, size_t data) {
    auto it = cacheMap.find(address);
    if (it == cacheMap.end()) {
        // No-write-allocate: não faz nada em caso de store miss
        return;
    }
    it->second.data = data;
    it->second.isDirty = true;
}

// --------------------------------------------------
void Cache::invalidate() {
    while (!fifo_queue.empty()) fifo_queue.pop();
    cacheMap.clear();
}

// --------------------------------------------------
std::vector<std::pair<size_t, size_t>> Cache::dirtyData() {
    std::vector<std::pair<size_t, size_t>> out;
    out.reserve(cacheMap.size());

    for (auto &kv : cacheMap) {
        const size_t addr = kv.first;
        const CacheEntry &entry = kv.second;

        if (entry.isDirty && entry.isValid) {
            out.emplace_back(addr, entry.data);
        }
    }
    return out;
}

// --------------------------------------------------
int Cache::get_misses() { return cache_misses; }
int Cache::get_hits()   { return cache_hits; }
