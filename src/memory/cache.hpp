#ifndef CACHE_HPP
#define CACHE_HPP

#include <unordered_map>
#include <queue>
#include <list>
#include <vector>
#include <cstddef>

#include "constants.hpp"


class MemoryManager;

enum class CachePolicyType {
    FIFO,
    LRU
};
struct CacheEntry {
    size_t data = 0;
    bool isValid = false;
    bool isDirty = false;
};

class Cache {
private:
    size_t capacity;
    CachePolicyType policy;

    // Mapeamento address → entrada
    std::unordered_map<size_t, CacheEntry> cacheMap;

    // FIFO
    std::queue<size_t> fifo_queue;

    // LRU
    std::list<size_t> lru_list;  
    std::unordered_map<size_t, std::list<size_t>::iterator> lru_pos;

    // Métricas
    int cache_hits;
    int cache_misses;

    void evictEntry(MemoryManager* memManager);

public:
    Cache(size_t capacity_, CachePolicyType p = CachePolicyType::FIFO);
    ~Cache();

    size_t get(size_t address);
    void put(size_t address, size_t data, MemoryManager* memManager);
    void update(size_t address, size_t data);
    void invalidate();
    std::vector<std::pair<size_t, size_t>> dirtyData();

    int get_hits();
    int get_misses();
};

#endif
