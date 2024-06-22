#pragma once
#include <optional>

#include "instructions.h"

struct ROBStatusBundle {
    // ALU section
    unsigned result;
    // BRU section
    bool mispredict, actualTaken;
    unsigned jumpTarget;
    bool ready;
    // LSU section
    bool cacheHit;
};

struct ROBStatusWritePort {
    unsigned result;
    bool mispredict, actualTaken;
    unsigned jumpTarget;
    unsigned robIdx;
    bool cacheHit;
};

struct ROBEntry {
    Instruction inst{};
    ROBStatusBundle state{};
    bool valid = false;
};

class ReorderBuffer {
    ROBEntry buffer[ROB_SIZE];
    unsigned pushPtr, popPtr;  // [popPtr, pushPtr)

public:
    ReorderBuffer();
    [[nodiscard]] bool canPush() const;
    [[nodiscard]] bool canPop() const;
    unsigned push(const Instruction &x, bool ready);
    void pop();
    void flush();
    [[nodiscard]] std::optional<ROBEntry> getFront() const;
    void writeState(const ROBStatusWritePort &x);
    [[nodiscard]] unsigned getPopPtr() const;
    [[nodiscard]] unsigned read(unsigned addr) const;
    [[nodiscard]] bool checkReady(unsigned addr) const;
	void showContent();
};
