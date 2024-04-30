#pragma once

#include "processor.h"
#include "rob.h"

class BackendWithCache : public Backend {
    Cache dcache;
    unsigned long totalMemoryTime, totalCacheHitTime;

protected:
    std::optional<ROBStatusWritePort> execute(
        ExecutePipeline &pipeline) override;
    void flush() override;

public:
    unsigned long getTotalMemoryTime() const { return totalMemoryTime; }
    unsigned long getTotalCacheHitTime() const { return totalCacheHitTime; }

public:
    BackendWithCache(const std::vector<unsigned> &data,
                     RegisterFile *reg,
                     unsigned memoryLatency,
                     unsigned cacheSize,
                     unsigned cacheBlockSize,
                     unsigned cacheAssociativity,
                     bool cacheWriteThrough,
                     ReplaceType cacheReplaceType);
    [[nodiscard]] unsigned read(unsigned addr) const override;
    bool writeMemoryHierarchy(unsigned address,
                              unsigned data,
                              unsigned byteEnable) override;
    bool commitInstruction(const ROBEntry &entry, Frontend &frontend) override;

    void reset(const std::vector<unsigned> &data) override;
};

class ProcessorWithCache : public ProcessorAbstract {
    RegisterFile regFile;
    Frontend frontend;
    BackendWithCache backend;

public:
    ProcessorWithCache(const std::vector<unsigned> &inst,
                       const std::vector<unsigned> &data,
                       unsigned entry,
                       unsigned memoryLatency,
                       unsigned cacheSize,
                       unsigned cacheBlockSize,
                       unsigned cacheAssociativity,
                       bool cacheWriteThrough,
                       ReplaceType cacheReplaceType);
    bool step() override;
    [[nodiscard]] unsigned readMem(unsigned addr) const;
    [[nodiscard]] unsigned readReg(unsigned addr) const;

    void loadProgram(const std::vector<unsigned> &inst,
                     const std::vector<unsigned> &data,
                     unsigned entry) override;
    void writeReg(unsigned addr, unsigned value) override;
    void writeMem(unsigned addr, unsigned value) override;

    unsigned long getTotalMemoryTime() const {
        return backend.getTotalMemoryTime();
    }
    unsigned long getTotalCacheHitTime() const {
        return backend.getTotalCacheHitTime();
    }
};