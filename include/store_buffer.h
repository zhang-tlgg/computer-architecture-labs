#pragma once

#include <optional>

#include "defines.h"

struct StoreBufferSlot {
    unsigned storeAddress;
    unsigned storeData;
    unsigned robIdx;
    bool valid;
};

class StoreBuffer {
    StoreBufferSlot buffer[ROB_SIZE];
    unsigned pushPtr, popPtr;

public:
    StoreBuffer();
    void push(unsigned addr, unsigned value, unsigned robIdx);
    StoreBufferSlot pop();
    StoreBufferSlot front();
    void flush();
    std::optional<unsigned> query(unsigned addr, unsigned robIdx, unsigned robPopPtr);
};
