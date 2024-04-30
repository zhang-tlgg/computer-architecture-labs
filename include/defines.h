#pragma once

#include <string>
#include <vector>

constexpr int XLEN = 32;

enum class InstructionType { R, I, S, B, J, U };

extern std::string xreg_name[32];
extern std::string freg_name[32];

unsigned readElf(const std::string &elfFileName,
                 std::vector<unsigned> &instruction,
                 std::vector<unsigned> &data);

// 4MB size
constexpr unsigned INST_MEM_SIZE = 0x400000u;
// 4MB size
constexpr unsigned DATA_MEM_SIZE = 0x400000u;

enum class FUType { ALU, BRU, LSU, MUL, DIV, NONE };

constexpr unsigned ROB_SIZE = 16u;

constexpr unsigned MAX_CACHE_SIZE = 16384u;  // 16KB

unsigned log2(unsigned int x);