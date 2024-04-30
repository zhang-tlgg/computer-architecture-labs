#include "processor.h"

struct BTBEntry {
    unsigned pc;
    unsigned target;
    unsigned counter;
    bool valid;
};

class FrontendWithPredict : public Frontend {
    BTBEntry btb[1024];

protected:
    BranchPredictBundle bpuFrontendUpdate(unsigned int pc) override;

public:
    explicit FrontendWithPredict(const std::vector<unsigned> &inst);
    [[nodiscard]] unsigned calculateNextPC(unsigned pc) const override;
    void bpuBackendUpdate(const BpuUpdateData &x) override;

    void reset(const std::vector<unsigned> &inst, unsigned entry) override;
};

class ProcessorWithPredict : public ProcessorAbstract {
    RegisterFile regFile;
    FrontendWithPredict frontend;
    Backend backend;

public:
    ProcessorWithPredict(const std::vector<unsigned> &inst,
                         const std::vector<unsigned> &data,
                         unsigned entry,
                         unsigned memoryLatency);
    bool step() override;
    [[nodiscard]] unsigned readMem(unsigned addr) const;
    [[nodiscard]] unsigned readReg(unsigned addr) const;

    void loadProgram(const std::vector<unsigned> &inst,
                     const std::vector<unsigned> &data,
                     unsigned entry) override;

    void writeReg(unsigned addr, unsigned value) override;
    void writeMem(unsigned addr, unsigned value) override;
};
