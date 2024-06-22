#include "rob.h"

#include <stdexcept>
#include <sstream>

#include "logger.h"

ReorderBuffer::ReorderBuffer() {
    for (auto &x : buffer) {
        x.valid = false;
    }
}

/**
 * @brief 检查rob是否有空间流入
 *
 * @return true
 * @return false
 */
bool ReorderBuffer::canPush() const {
    bool full = ((pushPtr == popPtr) && buffer[pushPtr].valid);
    return !full;
}

/**
 * @brief 检查rob是否有指令可以流出，不检查该指令是否已完成
 *
 * @return true
 * @return false
 */
bool ReorderBuffer::canPop() const {
    bool empty = ((pushPtr == popPtr) && !buffer[popPtr].valid);
    return !empty;
}

/**
 * @brief 向ROB中推入新指令，使用前应检查 canPush
 *
 * @param x 指令
 * @param ready 该指令是否在进入ROB时就标记为已完成
 * @return unsigned 该指令对应的 rob entry 编号
 */
unsigned ReorderBuffer::push(const Instruction &x, bool ready) {
    if (!canPush()) {
        Logger::Error("ReorderBuffer::push: ROB full!");
        throw std::runtime_error("ROB full when push!");
    }
    buffer[pushPtr].valid = true;
    buffer[pushPtr].inst = x;
    buffer[pushPtr].state.ready = ready;
    unsigned ret = pushPtr;
    pushPtr++;
    if (pushPtr == ROB_SIZE) pushPtr -= ROB_SIZE;
    return ret;
}

/**
 * @brief 从ROB中弹出指令，使用前应检查 canPop，以及指令是否完成
 *
 */
void ReorderBuffer::pop() {
    if (!canPop()) {
        Logger::Error("ReorderBuffer::pop: ROB empty!");
        throw std::runtime_error("ROB empty when pop!");
    }
    if (!buffer[popPtr].state.ready) {
        Logger::Warn("ReorderBuffer::pop: ROB pop when entry is not ready!");
    }
    buffer[popPtr].valid = false;
    popPtr++;
    if (popPtr == ROB_SIZE) popPtr -= ROB_SIZE;
}

/**
 * @brief 清空ROB
 *
 */
void ReorderBuffer::flush() {
    for (auto &x : buffer) {
        x.valid = false;
    }
    pushPtr = popPtr = 0;
}

std::optional<ROBEntry> ReorderBuffer::getFront() const {
    if (!canPop()) {
        return std::nullopt;
    }
    return std::make_optional<ROBEntry>(buffer[popPtr]);
}

void ReorderBuffer::writeState(const ROBStatusWritePort &x) {
    Logger::Info("Writing entry %u\n", x.robIdx);
    if (!buffer[x.robIdx].valid) {
        Logger::Error(
            "ReorderBuffer::writeState writing state into an invalid entry!");
        throw std::runtime_error("Writing state into invalid entry!");
    }
    buffer[x.robIdx].state.actualTaken = x.actualTaken;
    buffer[x.robIdx].state.mispredict = x.mispredict;
    buffer[x.robIdx].state.result = x.result;
    buffer[x.robIdx].state.jumpTarget = x.jumpTarget;
    buffer[x.robIdx].state.ready = true;
    buffer[x.robIdx].state.cacheHit = x.cacheHit;
}

unsigned ReorderBuffer::getPopPtr() const { return popPtr; }

unsigned ReorderBuffer::read(unsigned addr) const {
    return buffer[addr].state.result;
}

bool ReorderBuffer::checkReady(unsigned addr) const {
    return buffer[addr].state.ready;
}

void ReorderBuffer::showContent() {
	std::stringstream ss;
	Logger::setDebugOutput(true);
	Logger::Debug("ROB: ");
    unsigned p = popPtr;
    while (p != pushPtr) {
        ROBEntry slot = buffer[p];
        ss << p << " " << slot.inst << " " << "\n";

        p++;
        if (p == ROB_SIZE)
            p = 0;
    }
	Logger::Debug("%s", ss.str().c_str());
}
