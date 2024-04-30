#include "cache.h"

#include <cstring>
#include <optional>

#include "defines.h"
#include "logger.h"

CacheBlock::CacheBlock(size_t size) : size(size) {
    data = new unsigned char[size];
    valid = false;
    dirty = false;
    tag = 0u;
}

CacheBlock::CacheBlock(const CacheBlock &r)
    : size(r.size), tag(r.tag), valid(r.valid), dirty(r.dirty) {
    data = new unsigned char[size];
    memcpy(data, r.data, size);
}

CacheBlock::~CacheBlock() { delete[] data; }

CacheSet::CacheSet(size_t setSize, size_t blockSize) {
    for (unsigned i = 0; i < setSize; i++) {
        set.emplace_back(blockSize);
    }
}

CacheBlock &CacheSet::operator[](size_t index) {
    if (index >= set.size()) {
        Logger::Error("Index %lu out of range", index);
    }
    return set[index];
}

const CacheBlock &CacheSet::operator[](size_t index) const {
    if (index >= set.size()) {
        Logger::Error("Index %lu out of range", index);
    }
    return set[index];
}

Cache::Cache(unsigned size,
             unsigned blockSize,
             unsigned associativity,
             bool writeThrough,
             ReplaceType replaceType)
    : size(size),
      blockSize(blockSize),
      associativity(associativity),
      writeThrough(writeThrough),
      replaceType(replaceType) {
    unsigned setNum = size / blockSize / associativity;
    for (unsigned i = 0; i < setNum; i++) {
        cacheSets.emplace_back(associativity, blockSize);
        if (replaceType == ReplaceType::FIFO)
            fifoPointers.push_back(0);
        else if (replaceType == ReplaceType::LRU) {
            lruPointers.emplace_back();
            for (unsigned j = 0; j < associativity; j++)
                lruPointers.back().push_back(j);
        }
    }

    replaceID = -1u;
    saveOffset = -1u;

    occupied = false;
    occupyWriteFlag = false;
    occupyAddress = 0u;
}

/**
 * @brief 重置 Cache 的全部状态，invalid 所有表项
 *
 */
void Cache::reset() {
    while (!cacheSets.empty()) cacheSets.pop_back();
    while (!fifoPointers.empty()) fifoPointers.pop_back();
    while (!lruPointers.empty()) lruPointers.pop_back();

    unsigned setNum = size / blockSize / associativity;
    for (unsigned i = 0; i < setNum; i++) {
        cacheSets.emplace_back(associativity, blockSize);
        if (replaceType == ReplaceType::FIFO)
            fifoPointers.push_back(0);
        else if (replaceType == ReplaceType::LRU) {
            lruPointers.emplace_back();
            for (unsigned j = 0; j < associativity; j++)
                lruPointers.back().push_back(j);
        }
    }

    replaceID = -1u;
    saveOffset = -1u;

    occupied = false;
    occupyWriteFlag = false;
    occupyAddress = 0u;
}

/**
 * @brief 查询 Cache，多次请求可以交叉，但不会返回
 * 
 * @param physAddr 物理地址 (0x80400000u ~ 0x807FFFFCu)，4字节对齐
 * @param memory 使用的主存
 * @return std::nullopt 查询未完成
 * @return std::optional<unsigned> 查询结果
 */
std::optional<unsigned> Cache::query(unsigned physAddr,
                                     Memory &memory,
                                     bool &cacheHit) {
    if (occupied && (physAddr != occupyAddress || occupyWriteFlag)) {
        return std::nullopt;
    }

    occupied = true;
    occupyAddress = physAddr;
    occupyWriteFlag = false;

    // Split the address into tag, index and offset
    unsigned setNum = size / blockSize / associativity;
    unsigned offset = physAddr & (blockSize - 1u);
    unsigned index = (physAddr >> log2(blockSize)) & (setNum - 1u);
    unsigned tag = (physAddr >> log2(blockSize)) >> log2(setNum);

    Logger::Info("Address: 0x%08x, tag: 0x%08x, index: %d, offset: %d\n",
                 physAddr,
                 tag,
                 index,
                 offset);

    for (unsigned i = 0; i < associativity; i++) {
        if (cacheSets[index][i].valid && cacheSets[index][i].tag == tag) {
            Logger::Info("Query cache hit, index = %d", i);
            if (replaceType == ReplaceType::LRU) {
                auto it = lruPointers[index].begin();
                while (*it != i) it++;
                while (true) {
                    auto it1 = it;
                    it1++;
                    if (it1 == lruPointers[index].end()) break;
                    *it = *it1;
                    it++;
                }
                lruPointers[index].back() = i;
            }
            auto *addr =
                (unsigned *) (cacheSets[index][i].data + (offset & ~0x3u));
            occupied = false;
            cacheHit = true;
            return std::make_optional(*addr);
        }
    }

    if (replaceID == 0xFFFFFFFFu) {
        switch (replaceType) {
        case ReplaceType::FIFO:
            replaceID = fifoPointers[index];
            break;
        case ReplaceType::LRU:
            replaceID = lruPointers[index].front();
            break;
        case ReplaceType::RANDOM:
            replaceID = rand() % associativity;
            break;
        }
    }

    if (cacheSets[index][replaceID].valid &&
        cacheSets[index][replaceID].dirty) {
        if (saveOffset == -1u) saveOffset = 0;
        unsigned reconstructedAddr =
            (((cacheSets[index][replaceID].tag << log2(setNum)) | index)
             << log2(blockSize)) +
            saveOffset - 0x80400000u;
        bool finished = memory.write(
            reconstructedAddr >> 2u,
            *((unsigned *) (cacheSets[index][replaceID].data + saveOffset)),
            0xF);
        if (finished) saveOffset += 4;
        if (saveOffset == blockSize) {
            // cache block finished writing back
            cacheSets[index][replaceID].dirty = false;
            cacheSets[index][replaceID].valid = false;
            saveOffset = -1u;
        }
        return std::nullopt;
    }

    if (saveOffset == -1u) {
        saveOffset = 0u;
        cacheSets[index][replaceID].valid = false;
    }
    if (saveOffset != blockSize) {
        unsigned replaceAddr =
            (physAddr & (~(blockSize - 1u))) + saveOffset - 0x80400000u;
        Logger::Info(
            "saveOffset = %d, replaceAddr = 0x%08x", saveOffset, replaceAddr);
        auto result = memory.read(replaceAddr >> 2u);
        if (result.has_value()) {
            auto *tmp =
                (unsigned *) (cacheSets[index][replaceID].data + saveOffset);
            *tmp = result.value();
            saveOffset += 4;
            if (saveOffset == blockSize) {
                cacheSets[index][replaceID].valid = true;
                cacheSets[index][replaceID].dirty = false;
                cacheSets[index][replaceID].tag = tag;
                if (replaceType == ReplaceType::LRU) {
                    for (unsigned i = 0; i < lruPointers[index].size() - 1;
                         i++) {
                        lruPointers[index][i] = lruPointers[index][i + 1];
                    }
                    lruPointers[index].back() = replaceID;
                } else if (replaceType == ReplaceType::FIFO) {
                    (fifoPointers[index] += 1) &= (associativity - 1);
                }
            }
        }
    }

    if (!cacheSets[index][replaceID].valid) return std::nullopt;

    auto *addr =
        (unsigned *) (cacheSets[index][replaceID].data + (offset & ~0x3u));
    replaceID = -1u;
    saveOffset = -1u;
    occupied = false;
    cacheHit = false;
    return std::make_optional(*addr);
}

/**
 * @brief 仅查询 Cache，不替换，不访存
 * 
 * @param physAddr 物理地址 (0x80400000u ~ 0x807FFFFCu)，4字节对齐
 * @return std::optional<unsigned> 查询结果
 */
std::optional<unsigned> Cache::query(unsigned physAddr) const {
    // Split the address into tag, index and offset
    unsigned setNum = size / blockSize / associativity;
    unsigned offset = physAddr & (blockSize - 1u);
    unsigned index = (physAddr >> log2(blockSize)) & (setNum - 1u);
    unsigned tag = (physAddr >> log2(blockSize)) >> log2(setNum);

    for (unsigned i = 0; i < associativity; i++) {
        if (cacheSets[index][i].valid && cacheSets[index][i].tag == tag) {
            auto *addr =
                (unsigned *) (cacheSets[index][i].data + (offset & ~0x3u));
            return std::make_optional(*addr);
        }
    }

    return std::nullopt;
}

/**
 * @brief 写 Cache
 * 
 * @param physAddr 物理地址 (0x80400000u ~ 0x807FFFFCu)，4字节对齐
 * @param data 数据
 * @param memory 使用的主存
 * @param byteEnable 字节使能
 * @return true 完成写入
 * @return false 未完成写入
 */
bool Cache::write(unsigned int physAddr,
                  unsigned int data,
                  Memory &memory,
                  unsigned byteEnable,
                  bool &cacheHit) {
    if (occupied && (physAddr != occupyAddress || !occupyWriteFlag)) {
        return false;
    }

    occupied = true;
    occupyAddress = physAddr;
    occupyWriteFlag = true;

    // Split the address into tag, index and offset
    unsigned setNum = size / blockSize / associativity;
    unsigned offset = physAddr & (blockSize - 1u);
    unsigned index = (physAddr >> log2(blockSize)) & (setNum - 1u);
    unsigned tag = (physAddr >> log2(blockSize)) >> log2(setNum);

    Logger::Info(
        "205: Address: 0x%08x, tag: 0x%08x, index: %d, offset: %d, data: %d\n",
        physAddr,
        tag,
        index,
        offset,
        data);

    for (unsigned i = 0; i < associativity; i++) {
        Logger::Info("Checking index = %d", i);
        if (cacheSets[index][i].valid && cacheSets[index][i].tag == tag) {
            Logger::Info("Cache hit, index = %d", i);
            if (replaceType == ReplaceType::LRU) {
                auto it = lruPointers[index].begin();
                while (*it != i) it++;
                while (true) {
                    auto it1 = it;
                    it1++;
                    if (it1 == lruPointers[index].end()) break;
                    *it = *it1;
                    it++;
                }
                lruPointers[index].back() = i;
            }
            auto *addr = (cacheSets[index][i].data + (offset & ~0x3u));
            for (unsigned j = 0; j < 4; j++) {
                if (byteEnable & (1u << j)) {
                    *(addr + j) = (data >> (j * 8u)) & 0xffu;
                }
            }

            if (writeThrough) {
                Logger::Info("Writing through");
                if (memory.write(
                        (physAddr - 0x80400000u) >> 2u, data, byteEnable)) {
                    occupied = false;
                    cacheHit = true;
                    return true;
                }
                return false;
            } else {
                occupied = false;
                cacheSets[index][i].dirty = true;
                cacheHit = true;
                return true;
            }
        }
    }

    if (replaceID == 0xFFFFFFFFu) {
        switch (replaceType) {
        case ReplaceType::FIFO:
            replaceID = fifoPointers[index];
            break;
        case ReplaceType::LRU:
            replaceID = lruPointers[index].front();
            break;
        case ReplaceType::RANDOM:
            replaceID = rand() % associativity;
            break;
        }
    }

    Logger::Info("ReplaceID = %d, saveOffset = %d", replaceID, saveOffset);

    if (cacheSets[index][replaceID].valid &&
        cacheSets[index][replaceID].dirty) {
        if (saveOffset == -1u) saveOffset = 0;
        unsigned reconstructedAddr =
            (((cacheSets[index][replaceID].tag << log2(setNum)) | index)
             << log2(blockSize)) +
            saveOffset - 0x80400000u;
        Logger::Info("reconstructAddr = 0x%08x", reconstructedAddr);
        bool finished = memory.write(
            reconstructedAddr >> 2u,
            *((unsigned *) (cacheSets[index][replaceID].data + saveOffset)),
            0xF);
        if (finished) saveOffset += 4;
        if (saveOffset == blockSize) {
            // cache block finished writing back
            cacheSets[index][replaceID].dirty = false;
            cacheSets[index][replaceID].valid = false;
            saveOffset = -1u;
        }
        return false;
    }

    Logger::Info("ReplaceID = %d, saveOffset = %d", replaceID, saveOffset);

    if (saveOffset == -1u) {
        cacheSets[index][replaceID].valid = false;
        saveOffset = 0u;
    }
    if (saveOffset != blockSize) {
        unsigned replaceAddr =
            (physAddr & (~(blockSize - 1u))) + saveOffset - 0x80400000u;
        Logger::Info("replaceAddr = 0x%08x", replaceAddr);
        auto result = memory.read(replaceAddr >> 2u);
        if (result.has_value()) {
            Logger::Info("Finish replacing");
            auto *tmp =
                (unsigned *) (cacheSets[index][replaceID].data + saveOffset);
            *tmp = result.value();
            saveOffset += 4;
            if (saveOffset == blockSize) {
                cacheSets[index][replaceID].valid = true;
                cacheSets[index][replaceID].dirty = false;
                if (replaceType == ReplaceType::LRU) {
                    for (unsigned i = 0; i < lruPointers[index].size() - 1;
                         i++) {
                        lruPointers[index][i] = lruPointers[index][i + 1];
                    }
                    lruPointers[index].back() = replaceID;
                } else if (replaceType == ReplaceType::FIFO) {
                    (fifoPointers[index] += 1) &= (associativity - 1);
                }
            }
        }
    }

    if (!cacheSets[index][replaceID].valid) return false;

    auto *addr = (cacheSets[index][replaceID].data + (offset & ~0x3u));
    for (unsigned j = 0; j < 4; j++) {
        if (byteEnable & (1u << j)) {
            *(addr + j) = (data >> (j * 8u)) & 0xffu;
        }
    }

    if (writeThrough) {
        Logger::Info("Wait for possible write through.");
        if (memory.write((physAddr - 0x80400000u) >> 2u, data, byteEnable)) {
            cacheSets[index][replaceID].tag = tag;
            replaceID = -1u;
            saveOffset = -1u;
            occupied = false;
            cacheHit = false;
            return true;
        }
        return false;
    } else {
        occupied = false;
        cacheSets[index][replaceID].dirty = true;
        replaceID = -1u;
        saveOffset = -1u;
        cacheHit = false;
        return true;
    }
}

/**
 * @brief 仅重置 Cache 的当前请求状态
 * 用于刷新流水线
 */
void Cache::resetState() {
    occupied = false;
    replaceID = -1u;
    saveOffset = -1u;
}
