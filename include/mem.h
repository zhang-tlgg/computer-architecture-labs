#pragma once

#include <array>
#include <memory>
#include <optional>
#include <random>

class Memory {
    unsigned int *data;
    unsigned saveAddress;
    bool saveWriteFlag;
    unsigned remainingTime;

    const unsigned latency;

    std::default_random_engine engine;
    std::uniform_int_distribution<int> generator;

public:
    explicit Memory(unsigned latency, int seed = 0);
    ~Memory();
    // Returns std::nullopt if read is incomplete
    std::optional<unsigned> read(unsigned address);
    // Returns false if write is incomplete
    bool write(unsigned address, unsigned data, unsigned byteEnable);

    // used for check and testing
    void functionalWrite(unsigned address, std::vector<unsigned> data);

    // used for check and testing
    [[nodiscard]] std::vector<unsigned> functionalRead(unsigned address,
                                                       unsigned length) const;

    void resetState();
};