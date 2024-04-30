#include "register_file.h"

#include <cstring>

RegisterFile::RegisterFile() {
    for (unsigned int &i : reg) i = 0;
    reg[2] = 0x80800000;  // initialize sp
    reg[3] = 0x80400000;  // initialize gp
}

/**
 * @brief 寄存器读
 *
 * @param addr
 * @return unsigned
 */
unsigned RegisterFile::read(unsigned addr) const { return reg[addr]; }

void RegisterFile::functionalWrite(unsigned addr, unsigned value) {
    reg[addr] = value;
}

/**
 * @brief 寄存器写，清busy
 *
 * @param addr
 * @param val
 */
void RegisterFile::write(unsigned addr, unsigned val, unsigned robIdx) {
    if (addr != 0) {
        // busy[addr] = false;
        if (robIdx == robIndices[addr]) busy[addr] = false;
        reg[addr] = val;
    }
}

/**
 * @brief 获取寄存器的busy状态
 *
 * @param addr
 * @return true
 * @return false
 */
bool RegisterFile::isBusy(unsigned addr) const {
    return addr == 0 ? false : busy[addr];
}

/**
 * @brief 将寄存器标记为busy
 *
 * @param addr
 * @param robIdx 对应的 ROB 号
 */
void RegisterFile::markBusy(unsigned addr, unsigned robIdx) {
    busy[addr] = true;
    robIndices[addr] = robIdx;
}

/**
 * @brief 清空寄存器busy标记
 *
 */
void RegisterFile::flush() { memset(busy, false, sizeof(busy)); }

/**
 * @brief 获取寄存器对应的 ROB 编号
 *
 * @param addr
 * @return unsigned
 */
unsigned RegisterFile::getBusyIndex(unsigned addr) const {
    return robIndices[addr];
}

void RegisterFile::reset() {
    for (unsigned int &i : reg) i = 0;
    reg[2] = 0x80800000;  // initialize sp
    reg[3] = 0x80400000;  // initialize gp
    memset(busy, false, sizeof(busy));
}
