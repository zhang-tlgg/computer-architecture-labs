#include <stdexcept>

#include "rob.h"
#include "with_cache.h"

BackendWithCache::BackendWithCache(const std::vector<unsigned> &data,
                                   RegisterFile *reg,
                                   unsigned memoryLatency,
                                   unsigned cacheSize,
                                   unsigned cacheBlockSize,
                                   unsigned cacheAssociativity,
                                   bool cacheWriteThrough,
                                   ReplaceType cacheReplaceType)
    : Backend(data, reg, memoryLatency),
      dcache(cacheSize,
             cacheBlockSize,
             cacheAssociativity,
             cacheWriteThrough,
             cacheReplaceType),
      totalMemoryTime(0),
      totalCacheHitTime(0) {}

std::optional<ROBStatusWritePort> BackendWithCache::execute(
    ExecutePipeline &pipeline) {
    auto tmp = pipeline.step(dcache, memory, loadBuffer, rob, storeBuffer);
    return tmp;
}

unsigned BackendWithCache::read(unsigned addr) const {
    auto tmp = dcache.query(addr);
    return tmp.value_or(
        memory.functionalRead((addr - 0x80400000u) >> 2u, 1)[0]);
}

bool BackendWithCache::writeMemoryHierarchy(unsigned int address,
                                            unsigned int data,
                                            unsigned int byteEnable) {
    Logger::Info("Writing memory hierarchy address = 0x%08x, data = %d\n",
                 address,
                 data);
    bool cacheHit;
    bool flag = dcache.write(address, data, memory, byteEnable, cacheHit);
    if (flag) {
        if (dcache.query(address) != data) {
            Logger::Error("Store to cache failed");
            throw std::runtime_error("Store to cache failed");
        }

        totalMemoryTime += 1;
        if (cacheHit) {
            totalCacheHitTime += 1;
        }
    }
    return flag;
}

bool BackendWithCache::commitInstruction(const ROBEntry &entry,
                                     Frontend &frontend) {
    using namespace RV32I;

    unsigned saveROBPopPtr = rob.getPopPtr();

    bool executeExit = Backend::commitInstruction(entry, frontend);

    if (saveROBPopPtr != rob.getPopPtr() &&
        (entry.inst == LB || entry.inst == LBU || entry.inst == LH ||
        entry.inst == LHU || entry.inst == LW)) {
        auto ldSlot = loadBuffer.pop(saveROBPopPtr);
        if (!ldSlot.invalidate) {
            totalMemoryTime++;
            if (entry.state.cacheHit) {
                totalCacheHitTime++;
            }
        }
    }

    return executeExit;
}

void BackendWithCache::flush() {
    Backend::flush();
    dcache.resetState();
}

void BackendWithCache::reset(const std::vector<unsigned int> &data) {
    Backend::reset(data);
    dcache.reset();
    totalCacheHitTime = 0;
    totalMemoryTime = 0;
}