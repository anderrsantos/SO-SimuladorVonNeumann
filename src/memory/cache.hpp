#pragma once
#include <unordered_map>
#include <queue>
#include <vector>
#include <cstddef>

#define CACHE_MISS SIZE_MAX

// Entrada da cache
struct CacheEntry {
    size_t data;
    bool isValid;
    bool isDirty;
};

class MemoryManager;

class Cache {
public:
    // Construtor com capacidade configurável
    Cache(size_t capacity);
    ~Cache();

    size_t get(size_t address);
    void put(size_t address, size_t data, MemoryManager* memManager);
    void update(size_t address, size_t data);

    void invalidate();
    std::vector<std::pair<size_t, size_t>> dirtyData();

    int get_misses();
    int get_hits();

private:
    size_t capacity;
    std::unordered_map<size_t, CacheEntry> cacheMap;
    std::queue<size_t> fifo_queue;

    int cache_hits;
    int cache_misses;
};
