#pragma once
#include "instructions.h"

struct RegReadBundle {
    bool waitForWakeup;
    unsigned robIdx;
    unsigned value;
};

struct IssueSlot {
    Instruction inst{};
    RegReadBundle readPort1{}, readPort2{};
    unsigned robIdx = 0u;
    bool busy = false;
};
