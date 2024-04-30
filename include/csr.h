#pragma once

constexpr unsigned MSTATUS = 0x300;
constexpr unsigned MIE = 0x304;
constexpr unsigned MEPC = 0x341;
constexpr unsigned MIP = 0x344;

struct CSR {
    unsigned value;
    void set(unsigned value);
    void clear(unsigned value);
    void write(unsigned value);
    unsigned execute(unsigned instruction, unsigned value, bool write);
};

