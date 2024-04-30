#pragma once

class RegisterFile {
    unsigned reg[32];
    bool busy[32] = {false};
    unsigned robIndices[32] = {0u};

public:
    RegisterFile();
    [[nodiscard]] unsigned read(unsigned addr) const;
    void write(unsigned addr, unsigned val, unsigned robIdx);
    void functionalWrite(unsigned addr, unsigned value);
    [[nodiscard]] bool isBusy(unsigned addr) const;
    void markBusy(unsigned addr, unsigned robIdx);
    [[nodiscard]] unsigned getBusyIndex(unsigned addr) const;
    void flush();

    void reset();
};