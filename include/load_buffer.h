#pragma once

#include <optional>

#include "defines.h"

struct LoadBufferSlot {
    unsigned robIdx;
    unsigned loadAddress;
    bool valid;
    bool invalidate;
};

class LoadBuffer {
    LoadBufferSlot buffer[ROB_SIZE];

public:
    LoadBuffer();
    void push(unsigned addr, unsigned robIdx);
    LoadBufferSlot pop(unsigned robIdx);

    void check(unsigned addr, unsigned robIdx, unsigned robPopPtr);
    void flush();
};
