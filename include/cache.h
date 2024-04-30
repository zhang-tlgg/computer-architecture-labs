#include <array>
#include <memory>
#include <optional>
#include <random>

#include "mem.h"

enum class ReplaceType { FIFO, LRU, RANDOM };

struct CacheBlock {
    size_t size;
    unsigned tag;
    unsigned char *data;
    bool valid;
    bool dirty;

    explicit CacheBlock(size_t size);

    CacheBlock(const CacheBlock &r);
    CacheBlock &operator=(const CacheBlock &r) = delete;

    ~CacheBlock();
};

struct CacheSet {
    std::vector<CacheBlock> set;

    CacheSet(size_t setSize, size_t blockSize);
    CacheBlock &operator[](size_t index);
    const CacheBlock &operator[](size_t index) const;
};

class Cache {
    std::vector<CacheSet> cacheSets;

    const unsigned size;
    const unsigned blockSize;
    const unsigned associativity;
    const bool writeThrough;
    const ReplaceType replaceType;

    unsigned replaceID;
    unsigned saveOffset;

    // Next replace pointers
    std::vector<unsigned> fifoPointers;
    // Elements closer to the front of the array is less recently used
    std::vector<std::vector<unsigned>> lruPointers;

    bool occupied;
    unsigned occupyAddress;
    bool occupyWriteFlag;

public:
    // Due to architecture limitations, cache is always write allocated.
    Cache(unsigned size,
          unsigned blockSize,
          unsigned associativity,
          bool writeThrough,
          ReplaceType replaceType);

    // send in aligned physical address and byteEnable
    std::optional<unsigned> query(unsigned physAddr,
                                  Memory &memory,
                                  bool &cacheHit);

    [[nodiscard]] std::optional<unsigned> query(unsigned physAddr) const;

    // send in aligned physical address and byteEnable
    bool write(unsigned physAddr,
               unsigned data,
               Memory &memory,
               unsigned byteEnable,
               bool &cacheHit);

    void resetState();

    void reset();
};
