#include <cstring>
#include <iostream>
#include <optional>

#include "defines.h"
#include "logger.h"
#include "mem.h"

Memory::Memory(unsigned latency, int seed)
    : latency(latency), engine(seed), generator(-1, 1) {
    data = new unsigned int[DATA_MEM_SIZE >> 2u];
    memset(data, 0, DATA_MEM_SIZE);

    saveAddress = 0xFFFFFFFFu;
    saveWriteFlag = false;
    remainingTime = 0;
}

Memory::~Memory() { delete[] data; }

/**
 * @brief 主存功能性写入，仅用于初始化和验证用
 * @warning 实际验收时，这个函数会进行加密（异或一个 magic number 等）
 * 
 * @param address 写入地址
 * @param data 写入数据
 */
void Memory::functionalWrite(unsigned int address,
                             std::vector<unsigned int> data) {
    for (unsigned i = 0; i < data.size() && address + i < (DATA_MEM_SIZE >> 2u);
         i++)
        this->data[address + i] = data[i];
}

/**
 * @brief 主存功能性读取，仅用于验证
 * @warning 实际验收时，这个函数会进行加密（异或一个 magic number 等）
 * 
 * @param address 读取地址
 * @param length 读取长度
 * @return std::vector<unsigned> 
 */
std::vector<unsigned> Memory::functionalRead(unsigned int address,
                                             unsigned int length) const {
    std::vector<unsigned> ret;
    for (unsigned i = 0; i < length; i++) {
        if (address + i < (DATA_MEM_SIZE >> 2u)) {
            ret.push_back(data[address + i]);
        } else {
            ret.push_back(0u);
        }
    }

    return ret;
}

/**
 * @brief 主存读取接口
 * 
 * @param address 主存地址，每个地址代表 4 字节
 * @return std::optional<unsigned> 读取结果
 * @return std::nullopt 读取未完成
 */
std::optional<unsigned> Memory::read(unsigned address) {
    if (address >= (DATA_MEM_SIZE >> 2u)) {
        return std::nullopt;
    }

    if (remainingTime != 0) {
        if (saveAddress != address || saveWriteFlag) {
            Logger::Info(
                "Currently running another request: address = 0x%08x, "
                "writeFlag = %d",
                saveAddress,
                saveWriteFlag);
            return std::nullopt;
        }

        remainingTime--;

        Logger::Info("Read Remaining time = %d", remainingTime);

        if (remainingTime == 0) {
            Logger::Info(
                "Reading Memory 0x%08x, data = %d", address, data[address]);
        }

        return remainingTime == 0 ? std::make_optional(data[address])
                                  : std::nullopt;
    }

    if (address == saveAddress || address == saveAddress + 1) {
        saveAddress = address;
        Logger::Info(
            "Reading Memory 0x%08x, data = %d", address, data[address]);
        // continuous access or repetitive access
        return std::make_optional(data[address]);
    }

    saveAddress = address;
    saveWriteFlag = false;
    remainingTime = std::max(0, generator(engine) + (int) latency - 1);

    if (remainingTime == 0) {
        Logger::Info(
            "Reading Memory 0x%08x, data = %d", address, data[address]);
    }

    return remainingTime == 0 ? std::make_optional(data[address])
                              : std::nullopt;
}

/**
 * @brief 主存写入接口
 * 
 * @param address 主存地址，每个地址代表 4 字节
 * @param data 写入数据
 * @param byteEnable 字节使能
 * @return true 完成写入
 * @return false 未完成写入
 */
bool Memory::write(unsigned address, unsigned data, unsigned byteEnable) {
    if (address >= (DATA_MEM_SIZE >> 2u)) {
        Logger::Error("Data Memory Access Address is out of range");
        throw std::runtime_error("Data Memory Access Address is out of range");
    }

    if (remainingTime != 0) {
        if (saveAddress != address || !saveWriteFlag) {
            return false;
        }

        remainingTime--;

        if (remainingTime == 0) {
            unsigned result = 0;
            for (unsigned i = 0; i < 4; i++) {
                if (byteEnable & (1u << i)) {
                    result |= ((data >> (i * 8u)) & 0xffu) << (i * 8u);
                } else {
                    result |= (((this->data[address]) >> (i * 8u)) & 0xffu)
                              << (i * 8u);
                }
            }
            this->data[address] = result;
        }
        if (remainingTime == 0) {
            Logger::Info("Writing Memory 0x%08x, data = %d", address, data);
        }
        return remainingTime == 0;
    }

    saveAddress = address;
    saveWriteFlag = true;

    remainingTime = std::max(0, generator(engine) + (int) latency - 1);

    if (remainingTime == 0) {
        unsigned result = 0;
        for (unsigned i = 0; i < 4; i++) {
            if (byteEnable & (1u << i)) {
                result |= ((data >> (i * 8u)) & 0xffu) << (i * 8u);
            } else {
                result |= (((this->data[address]) >> (i * 8u)) & 0xffu)
                          << (i * 8u);
            }
        }
        this->data[address] = result;
    }
    if (remainingTime == 0) {
        Logger::Info("Writing Memory 0x%08x, data = %d", address, data);
    }
    return remainingTime == 0;
}

/**
 * @brief 重置主存的访问状态，用于刷新流水线
 * 
 */
void Memory::resetState() { remainingTime = 0; }
