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
    bool executeExit = false;
    using namespace RV32I;

    std::stringstream ss;
    ss << entry.inst;

    Logger::Info("Committing instruction %s: ", ss.str().c_str());
    Logger::Info("ROB index: %u", rob.getPopPtr());
    Logger::Info("rd: %s, value = %u",
                 xreg_name[entry.inst.getRd()].c_str(),
                 entry.state.result);

    StoreBufferSlot stSlot{};
    LoadBufferSlot ldSlot{};

    ldSlot.valid = false;
    ldSlot.invalidate = false;

    if (entry.inst == SB || entry.inst == SH || entry.inst == SW) {
        stSlot = storeBuffer.front();
        bool status =
            writeMemoryHierarchy(stSlot.storeAddress, stSlot.storeData, 0xF);
        if (!status) {
            return false;
        } else {
            storeBuffer.pop();
            Logger::Info("PC = 0x%08x, store to 0x%08x, data = %d",
                         entry.inst.pc,
                         stSlot.storeAddress,
                         stSlot.storeData);

            totalMemoryTime += 1;
            if (entry.state.cacheHit) {
                totalCacheHitTime += 1;
            }
        }
    } else if (entry.inst == LB || entry.inst == LBU || entry.inst == LH ||
               entry.inst == LHU || entry.inst == LW) {
        ldSlot = loadBuffer.pop(rob.getPopPtr());
        if (ldSlot.invalidate) {
            Logger::Info("Out of order load at PC = 0x08x", entry.inst.pc);
            frontend.jump(entry.inst.pc);
            flush();
            return false;
        }

        totalMemoryTime += 1;
        if (entry.state.cacheHit) {
            totalCacheHitTime += 1;
        }
    }

    if (entry.inst == EXTRA::EXIT) {
        executeExit = true;
    }

    regFile->write(entry.inst.getRd(), entry.state.result, rob.getPopPtr());

    if (entry.inst.getRd() != 0u)
        Logger::Info("PC = 0x%08x, write reg %d, data = %d",
                     entry.inst.pc,
                     entry.inst.getRd(),
                     entry.state.result);

    BpuUpdateData bpuUpdate{};
    bpuUpdate.pc = entry.inst.pc;
    bpuUpdate.isCall = entry.inst == JAL && entry.inst.getRd() == 1;
    bpuUpdate.isReturn = entry.inst == JALR && entry.inst.getRs1() == 1;
    bpuUpdate.isBranch = getFUType(entry.inst) == FUType::BRU &&
                         entry.inst != JAL && entry.inst != JALR;
    bpuUpdate.branchTaken = entry.state.actualTaken;
    bpuUpdate.jumpTarget = entry.state.jumpTarget;
    frontend.bpuBackendUpdate(bpuUpdate);

    rob.pop();
    if (entry.state.mispredict) {
        Logger::Info("PC = 0x%08x, jump to 0x%08x",
                     entry.inst.pc,
                     entry.state.actualTaken ? entry.state.jumpTarget
                                             : entry.inst.pc + 4);
        frontend.jump(entry.state.actualTaken ? entry.state.jumpTarget
                                              : entry.inst.pc + 4);
        flush();
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