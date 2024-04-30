#include "logger.h"
#include "processor.h"

Frontend::Frontend(const std::vector<unsigned> &inst) {
    for (unsigned long p = 0; p < inst.size(); p++) {
        data[p] = inst[p];
    }
    data[inst.size()] = 0x0000000b;
}

/**
 * @brief 前端步进函数
 *
 * @return std::optional<Instruction>
 * 前端有指令需要流出时，返回指令，否则返回std::nullopt
 */
std::optional<Instruction> Frontend::step() {
    Logger::Info("Dispatch halt: %s", dispatchHalt ? "true" : "false");
    if (DISPATCH == std::nullopt || !dispatchHalt) {
        DISPATCH = ID;
        ID = std::nullopt;
    }
    dispatchHalt = false;
    auto instruction = DISPATCH;
    if (ID == std::nullopt) {
        ID = IF2;
        IF2 = std::nullopt;
    }
    if (IF2 == std::nullopt) {
        IF2 = IF1;
        if (IF2.has_value())
            IF2.value().predictBundle = bpuFrontendUpdate(IF2.value().pc);
        if (IF1 != EXTRA::EXIT) {
            IF1 = std::nullopt;
        } else {
            Logger::Info("Fetching EXIT, Frontend pipeline stalled!");
        }
    }
    if (IF1 == std::nullopt) {
        auto tmp = Instruction(data[(pc - 0x80000000u) >> 2u]);
        tmp.pc = pc;
        IF1 = std::make_optional<Instruction>(tmp);
        pc = calculateNextPC(pc);
    }
    return instruction;
}

/**
 * @brief 后端发生跳转的回调函数，清空流水线，修改nextpc
 *
 * @param jumpAddress 跳转地址
 */
void Frontend::jump(unsigned int jumpAddress) {
    Logger::Info("Jumped to %08x", jumpAddress);
    DISPATCH = std::nullopt;
    ID = std::nullopt;
    IF2 = std::nullopt;
    IF1 = std::make_optional<Instruction>(
        data[(jumpAddress - 0x80000000u) >> 2u]);
    IF1.value().pc = jumpAddress;
    pc = jumpAddress;
    pc = calculateNextPC(pc);
    Logger::Info("New PC = %08x", pc);
}

/**
 * @brief 用于计算NextPC，分支预测时需要
 *
 * @param pc
 * @return unsigned
 */
unsigned Frontend::calculateNextPC(unsigned pc) const { return pc + 4; }

/**
 * @brief 获取指令的分支预测结果，分支预测时需要
 *
 * @param pc 指令的pc
 * @return BranchPredictBundle 分支预测的结构
 */
BranchPredictBundle Frontend::bpuFrontendUpdate(
    [[maybe_unused]] unsigned int pc) {
    // do nothing
    BranchPredictBundle result;
    result.predictJump = false;
    return result;
}

/**
 * @brief 用于后端提交指令时更新分支预测器状态，分支预测时需要
 *
 * @param x
 */
void Frontend::bpuBackendUpdate([[maybe_unused]] const BpuUpdateData &x) {
    // do nothing
}

/**
 * @brief 当后端未接受指令时，阻止前端指令流出
 *
 */
void Frontend::haltDispatch() { dispatchHalt = true; }

/**
 * @brief 重置前端状态
 * 
 * @param inst 
 * @param entry 
 */
void Frontend::reset(const std::vector<unsigned> &inst, unsigned entry) {
    for (unsigned long p = 0; p < inst.size(); p++) {
        data[p] = inst[p];
    }
    data[inst.size()] = 0x0000000b;
    jump(entry);
}
