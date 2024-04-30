#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cache.h"
#include "defines.h"
#include "instructions.h"
#include "load_buffer.h"
#include "register_file.h"
#include "reservation_station.hpp"
#include "rob.h"
#include "store_buffer.h"

class Frontend {
    unsigned int pc = 0x80000000;
    unsigned data[INST_MEM_SIZE >> 2u];

    bool dispatchHalt = false;

    std::optional<Instruction> IF1, IF2, ID, DISPATCH;

protected:
    virtual BranchPredictBundle bpuFrontendUpdate(unsigned int pc);

public:
    explicit Frontend(const std::vector<unsigned> &inst);
    virtual std::optional<Instruction> step() final;
    virtual void jump(unsigned int jumpAddress) final;
    virtual void haltDispatch() final;
    [[nodiscard]] virtual unsigned calculateNextPC(unsigned pc) const;
    virtual void bpuBackendUpdate(const BpuUpdateData &x);

    virtual void reset(const std::vector<unsigned> &inst, unsigned entry);
};

class ExecutePipeline {
    const std::string name;
    IssueSlot executeSlot;
    unsigned counter;

public:
    explicit ExecutePipeline(std::string name);
    std::optional<ROBStatusWritePort> step(Memory &memory,
                                           LoadBuffer &ldBuf,
                                           ReorderBuffer &rob,
                                           StoreBuffer &stBuf);
    std::optional<ROBStatusWritePort> step(Cache &cache,
                                           Memory &memory,
                                           LoadBuffer &ldBuf,
                                           ReorderBuffer &rob,
                                           StoreBuffer &stBuf);
    void execute(const IssueSlot &x);
    [[nodiscard]] bool canExecute() const;
    void flush();
};

class Backend {
    ExecutePipeline alu, bru, lsu, mul, div;

protected:
    ReorderBuffer rob;
    ReservationStation<4> rsALU, rsBRU, rsMUL, rsDIV;
    ReservationStation<4> rsLSU;
    StoreBuffer storeBuffer;
    LoadBuffer loadBuffer;

    RegisterFile *const regFile;

    Memory memory;

    virtual std::optional<ROBStatusWritePort> execute(
        ExecutePipeline &pipeline);
    virtual void flush();
    virtual bool writeMemoryHierarchy(unsigned address,
                                      unsigned data,
                                      unsigned byteEnable);

public:
    Backend(const std::vector<unsigned> &data,
            RegisterFile *reg,
            unsigned memoryLatency);
    bool dispatchInstruction(const Instruction &inst);
    bool step(Frontend &frontend);
    [[nodiscard]] virtual unsigned read(unsigned addr) const;
    virtual bool commitInstruction(const ROBEntry &entry, Frontend &frontend);

    virtual void reset(const std::vector<unsigned> &data);
    void functionalWrite(unsigned addr, unsigned value);
};

class ProcessorAbstract {
public:
    virtual bool step() = 0;
    virtual void loadProgram(const std::vector<unsigned> &inst,
                             const std::vector<unsigned> &data,
                             unsigned entry) = 0;
    virtual void writeReg(unsigned addr, unsigned value) = 0;
    virtual void writeMem(unsigned addr, unsigned value) = 0;
};

class Processor : public ProcessorAbstract {
    // NOTE: Order is crucial, prevent initialization reordering
    RegisterFile regFile;
    Frontend frontend;
    Backend backend;

public:
    Processor(const std::vector<unsigned> &inst,
              const std::vector<unsigned> &data,
              unsigned entry,
              unsigned memoryLatency);

    bool step() override;
    [[nodiscard]] unsigned readMem(unsigned addr) const;
    [[nodiscard]] unsigned readReg(unsigned addr) const;

    void writeReg(unsigned addr, unsigned value) override;
    void writeMem(unsigned addr, unsigned value) override;

    void loadProgram(const std::vector<unsigned> &inst,
                     const std::vector<unsigned> &data,
                     unsigned entry) override;
};
