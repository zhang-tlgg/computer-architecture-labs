#include <sstream>
#include <stdexcept>
#include <utility>

#include "logger.h"
#include "processor.h"

ExecutePipeline::ExecutePipeline(std::string name) : name(std::move(name)) {
    counter = 0;
}

/**
 * @brief 用于执行一条新的指令，调用前应首先检查是否可以执行
 *
 * @param x 即将进入新指令发射槽
 */
void ExecutePipeline::execute(const IssueSlot &x) {
    if (executeSlot.busy) {
        Logger::Error("ExecutePipeline %s: execute slot busy", name.c_str());
        throw std::runtime_error("Execute slot busy");
    }
    executeSlot = x;
    if (name == "ALU") {
        counter = 1;
    } else if (name == "BRU") {
        counter = 1;
    } else if (name == "LSU") {
        counter = 2;
    } else if (name == "MUL") {
        counter = 3;
    } else if (name == "DIV") {
        counter = 10;
    } else {
        Logger::Error("Unknown execute pipeline %s", name.c_str());
        throw std::runtime_error("Unexpected execute pipeline name");
    }
}

/**
 * @brief 后端流水线步进函数
 *
 * @param dataMem 传入的data memory数组，用于ls指令读取数据
 * @param stBuf store buffer 模块，用于store指令调用
 * @return std::optional<ROBStatusWritePort>
 * 当指令执行完成时，会返回一个ROB写口，相当于向cdb写入信息。其余时刻返回std::nullopt
 */
std::optional<ROBStatusWritePort> ExecutePipeline::step(Memory &memory,
                                                        LoadBuffer &ldBuf,
                                                        ReorderBuffer &rob,
                                                        StoreBuffer &stBuf) {
    if (!executeSlot.busy) {
        return std::nullopt;
    }

    if (counter != 0) counter--;

    std::stringstream ss;
    ss << executeSlot.inst;

    Logger::Info("Execute pipeline %s:", name.c_str());
    Logger::Info("Running %s", ss.str().c_str());
    Logger::Info("ROB Index: %u", executeSlot.robIdx);
    Logger::Info("Operand1: %u, Operand2: %u",
                 executeSlot.readPort1.value,
                 executeSlot.readPort2.value);
    Logger::Info("Time Remaining: %d\n", counter);

    if (counter == 0) {
        executeSlot.busy = false;
        ROBStatusWritePort result{};
        ExecuteResultBundle exe = executeSlot.inst.execute(
            name, executeSlot.readPort1.value, executeSlot.readPort2.value);
        if (name == "LSU") {
            auto const &inst = executeSlot.inst;
            using namespace RV32I;
            if (inst == LB || inst == LH || inst == LW || inst == LBU ||
                inst == LHU) {
                unsigned loadResult;
                auto val = stBuf.query(
                    exe.result, executeSlot.robIdx, rob.getPopPtr());
                if (!val.has_value()) {
                    if (exe.result >= 0x80800000u || exe.result < 0x80400000u) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    }
                    auto tmp = memory.read((exe.result - 0x80400000u) >> 2u);
                    if (!tmp.has_value()) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    }
                    loadResult = tmp.value();
                } else {
                    loadResult = val.value();
                }
                if (inst == LW) {
                } else if (inst == LH || inst == LHU) {
                    if (exe.result & 2u) {
                        loadResult >>= 16u;
                    } else {
                        loadResult &= 0xFFFFu;
                    }
                    if (inst == LH && (loadResult & 0x8000u)) {
                        loadResult |= 0xFFFF0000u;
                    }
                } else if (inst == LB || inst == LBU) {
                    unsigned offset = exe.result & 0x3u;
                    unsigned shift = offset << 3u;
                    loadResult = (loadResult >> shift) & 0xFFu;
                    if (inst == LB && (loadResult & 0x80u)) {
                        loadResult |= 0xFFFFFF00u;
                    }
                }

                ldBuf.push(exe.result, executeSlot.robIdx);

                result.actualTaken = exe.actualTaken;
                result.mispredict = exe.mispredict;
                result.result = loadResult;
                result.jumpTarget = exe.jumpTarget;
                result.robIdx = executeSlot.robIdx;
                return std::make_optional(result);
            } else {
                // NOTE: Check load buffer ordering
                // If a following load instruction is executed before the store
                // Then it should be refetched and re-executed
                // Re-use mispredict as a flag for load instructions

                ldBuf.check(exe.result, executeSlot.robIdx, rob.getPopPtr());

                auto val = stBuf.query(
                    exe.result, executeSlot.robIdx, rob.getPopPtr());
                unsigned originalValue;
                if (!val.has_value()) {
                    if (exe.result >= 0x80800000u || exe.result < 0x80400000u) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    }
                    auto tmp = memory.read((exe.result - 0x80400000u) >> 2u);
                    Logger::Info("Memory Read Flag: %s\n",
                                 tmp.has_value() ? "true" : "false");
                    if (!tmp.has_value()) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    } else {
                        originalValue = tmp.value();
                    }
                } else {
                    originalValue = val.value();
                }
                if (inst == SW) {
                    originalValue = executeSlot.readPort2.value;
                } else if (inst == SH) {
                    if (exe.result & 2u) {
                        originalValue &= 0xFFFFu;
                        originalValue |= (executeSlot.readPort2.value << 16u);
                    } else {
                        originalValue &= 0xFFFF0000u;
                        originalValue |=
                            (executeSlot.readPort2.value & 0xFFFFu);
                    }
                } else if (inst == SB) {
                    unsigned offset = exe.result & 0x3u;
                    unsigned byte = executeSlot.readPort2.value & 0xFFu;
                    switch (offset) {
                    case 0:
                        originalValue &= 0xFFFFFF00u;
                        originalValue |= byte;
                        break;
                    case 1:
                        originalValue &= 0xFFFF00FFu;
                        originalValue |= byte << 8u;
                        break;
                    case 2:
                        originalValue &= 0xFF00FFFFu;
                        originalValue |= byte << 16u;
                        break;
                    case 3:
                        originalValue &= 0xFFFFFFu;
                        originalValue |= byte << 24u;
                        break;
                    default:
                        break;
                    }
                }
                stBuf.push(exe.result, originalValue, executeSlot.robIdx);

                result.actualTaken = exe.actualTaken;
                result.mispredict = exe.mispredict;
                result.result = exe.result;
                result.jumpTarget = exe.jumpTarget;
                result.robIdx = executeSlot.robIdx;
                return std::make_optional(result);
            }
        } else {
            result.actualTaken = exe.actualTaken;
            result.mispredict = exe.mispredict;
            result.result = exe.result;
            result.jumpTarget = exe.jumpTarget;
            result.robIdx = executeSlot.robIdx;
            return std::make_optional(result);
        }
    }
    return std::nullopt;
}

/**
 * @brief 后端流水线步进函数
 *
 * @param dataMem 传入的data memory数组，用于ls指令读取数据
 * @param stBuf store buffer 模块，用于store指令调用
 * @return std::optional<ROBStatusWritePort>
 * 当指令执行完成时，会返回一个ROB写口，相当于向cdb写入信息。其余时刻返回std::nullopt
 */
std::optional<ROBStatusWritePort> ExecutePipeline::step(Cache &cache,
                                                        Memory &memory,
                                                        LoadBuffer &ldBuf,
                                                        ReorderBuffer &rob,
                                                        StoreBuffer &stBuf) {
    if (!executeSlot.busy) {
        return std::nullopt;
    }

    if (counter != 0) counter--;

    std::stringstream ss;
    ss << executeSlot.inst;

    Logger::Info("Execute pipeline %s:", name.c_str());
    Logger::Info("Running %s", ss.str().c_str());
    Logger::Info("ROB Index: %u", executeSlot.robIdx);
    Logger::Info("Operand1: %u, Operand2: %u",
                 executeSlot.readPort1.value,
                 executeSlot.readPort2.value);
    Logger::Info("Time Remaining: %d\n", counter);

    if (counter == 0) {
        executeSlot.busy = false;
        ROBStatusWritePort result{};
        ExecuteResultBundle exe = executeSlot.inst.execute(
            name, executeSlot.readPort1.value, executeSlot.readPort2.value);
        bool cacheHit;
        if (name == "LSU") {
            auto const &inst = executeSlot.inst;
            using namespace RV32I;
            if (inst == LB || inst == LH || inst == LW || inst == LBU ||
                inst == LHU) {
                unsigned loadResult;
                auto val = stBuf.query(
                    exe.result, executeSlot.robIdx, rob.getPopPtr());
                if (!val.has_value()) {
                    if (exe.result >= 0x80800000u || exe.result < 0x80400000u) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    }
                    auto tmp =
                        cache.query(exe.result & 0xFFFFFFFCu, memory, cacheHit);
                    if (!tmp.has_value()) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    }
                    loadResult = tmp.value();
                } else {
                    loadResult = val.value();
                    // store-to-load forwarding is regarded as cache hit
                    cacheHit = true;
                }
                if (inst == LW) {
                } else if (inst == LH || inst == LHU) {
                    if (exe.result & 2u) {
                        loadResult >>= 16u;
                    } else {
                        loadResult &= 0xFFFFu;
                    }
                    if (inst == LH && (loadResult & 0x8000u)) {
                        loadResult |= 0xFFFF0000u;
                    }
                } else if (inst == LB || inst == LBU) {
                    unsigned offset = exe.result & 0x3u;
                    unsigned shift = offset << 3u;
                    loadResult = (loadResult >> shift) & 0xFFu;
                    if (inst == LB && (loadResult & 0x80u)) {
                        loadResult |= 0xFFFFFF00u;
                    }
                }
                ldBuf.push(exe.result, executeSlot.robIdx);
                result.actualTaken = exe.actualTaken;
                result.mispredict = exe.mispredict;
                result.result = loadResult;
                result.jumpTarget = exe.jumpTarget;
                result.robIdx = executeSlot.robIdx;
                result.cacheHit = cacheHit;
                return std::make_optional(result);
            } else {
                // NOTE: Check load buffer ordering
                // If a following load instruction is executed before the store
                // Then it should be refetched and re-executed
                // Re-use mispredict as a flag for load instructions

                ldBuf.check(exe.result, executeSlot.robIdx, rob.getPopPtr());

                auto val = stBuf.query(
                    exe.result, executeSlot.robIdx, rob.getPopPtr());
                unsigned originalValue;
                if (!val.has_value()) {
                    if (exe.result >= 0x80800000u || exe.result < 0x80400000u) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    }
                    auto tmp =
                        cache.query(exe.result & 0xFFFFFFFCu, memory, cacheHit);
                    Logger::Info("Memory Read Flag: %s\n",
                                 tmp.has_value() ? "true" : "false");
                    if (!tmp.has_value()) {
                        executeSlot.busy = true;
                        return std::nullopt;
                    } else {
                        originalValue = tmp.value();
                    }
                } else {
                    originalValue = val.value();
                    // store-to-load forwarding is regarded as cache hit
                    cacheHit = true;
                }
                if (inst == SW) {
                    originalValue = executeSlot.readPort2.value;
                } else if (inst == SH) {
                    if (exe.result & 2u) {
                        originalValue &= 0xFFFFu;
                        originalValue |= (executeSlot.readPort2.value << 16u);
                    } else {
                        originalValue &= 0xFFFF0000u;
                        originalValue |=
                            (executeSlot.readPort2.value & 0xFFFFu);
                    }
                } else if (inst == SB) {
                    unsigned offset = exe.result & 0x3u;
                    unsigned byte = executeSlot.readPort2.value & 0xFFu;
                    switch (offset) {
                    case 0:
                        originalValue &= 0xFFFFFF00u;
                        originalValue |= byte;
                        break;
                    case 1:
                        originalValue &= 0xFFFF00FFu;
                        originalValue |= byte << 8u;
                        break;
                    case 2:
                        originalValue &= 0xFF00FFFFu;
                        originalValue |= byte << 16u;
                        break;
                    case 3:
                        originalValue &= 0xFFFFFFu;
                        originalValue |= byte << 24u;
                        break;
                    default:
                        break;
                    }
                }
                stBuf.push(exe.result, originalValue, executeSlot.robIdx);

                result.actualTaken = exe.actualTaken;
                result.mispredict = exe.mispredict;
                result.result = exe.result;
                result.jumpTarget = exe.jumpTarget;
                result.robIdx = executeSlot.robIdx;
                result.cacheHit = cacheHit;
                return std::make_optional(result);
            }
        } else {
            result.actualTaken = exe.actualTaken;
            result.mispredict = exe.mispredict;
            result.result = exe.result;
            result.jumpTarget = exe.jumpTarget;
            result.robIdx = executeSlot.robIdx;
            return std::make_optional(result);
        }
    }
    return std::nullopt;
}

/**
 * @brief 检测流水线是否能流入新指令
 *
 * @return true
 * @return false
 */
bool ExecutePipeline::canExecute() const { return !executeSlot.busy; }

/**
 * @brief 清空流水线状态
 *
 */
void ExecutePipeline::flush() { executeSlot.busy = false; }
