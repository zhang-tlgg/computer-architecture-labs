#include "masked_literal.h"

#include <stdexcept>
#include <utility>

#include "defines.h"
#include "logger.h"

MaskedLiteral::MaskedLiteral(std::string s) : literal(std::move(s)) {
    if (literal.length() != XLEN) {
        Logger::Error("Literals' length must be 32 in RV32 Machines");
        throw std::invalid_argument(
            "Literals' length must be 32 in RV32 Machines");
    }
}

/**
 * @brief 用于检测指令编码是否与字符串的32位2进制相同
 *
 * @param s
 * @return true
 * @return false
 */
bool MaskedLiteral::operator==(const std::string &s) const {
    if (s.length() != XLEN) {
        Logger::Error("Instructions' length must be 32 in RV32 Machines");
        throw std::invalid_argument(
            "Instructions' length must be 32 in RV32 Machines");
    }
    for (int i = 0; i < XLEN; i++) {
        if (literal[i] == '-') {
            continue;
        } else if (literal[i] != s[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 用于检测指令编码是否与数字的32位2进制相同
 *
 * @param _x
 * @return true
 * @return false
 */
bool MaskedLiteral::operator==(const unsigned &_x) const {
    unsigned x = _x;
    std::string literal;
    while (x > 0) {
        literal.push_back((x & 1u) + '0');
        x >>= 1u;
    }
    while (literal.size() != 32) literal.push_back('0');
    literal.assign(literal.rbegin(), literal.rend());  // Reverse the order
    return (*this) == literal;
}

/**
 * @brief 用于检测指令编码是否与数字的32位2进制相同
 *
 * @param lhs
 * @param rhs
 * @return true
 * @return false
 */
bool operator==(const unsigned &lhs, const MaskedLiteral &rhs) {
    return rhs == lhs;
}

/**
 * @brief 用于生成U，J类指令编码
 *
 * @param opcode
 * @return MaskedLiteral
 */
MaskedLiteral MaskedLiteral::UType(std::string opcode) {
    std::string literal;
    for (int i = 0; i < 25; i++) literal.push_back('-');
    return MaskedLiteral(literal + opcode);
}

/**
 * @brief 用于生成I，B，S类指令编码
 *
 * @param opcode
 * @param funct3
 * @return MaskedLiteral
 */
MaskedLiteral MaskedLiteral::IType(std::string opcode, std::string funct3) {
    std::string literal;
    for (int i = 0; i < 17; i++) literal.push_back('-');
    return MaskedLiteral(literal + funct3 + "-----" + opcode);
}

/**
 * @brief 用于生成R类指令编码
 *
 * @param opcode
 * @param funct3
 * @param funct7
 * @return MaskedLiteral
 */
MaskedLiteral MaskedLiteral::RType(std::string opcode,
                                   std::string funct3,
                                   std::string funct7) {
    std::string literal;
    for (int i = 0; i < 10; i++) literal.push_back('-');
    return MaskedLiteral(funct7 + literal + funct3 + "-----" + opcode);
}

namespace RV32I {
const MaskedLiteral LUI = MaskedLiteral::UType("0110111");
const MaskedLiteral AUIPC = MaskedLiteral::UType("0010111");
const MaskedLiteral JAL = MaskedLiteral::UType("1101111");

const MaskedLiteral JALR = MaskedLiteral::IType("1100111", "000");

const MaskedLiteral BEQ = MaskedLiteral::IType("1100011", "000");
const MaskedLiteral BNE = MaskedLiteral::IType("1100011", "001");
const MaskedLiteral BLT = MaskedLiteral::IType("1100011", "100");
const MaskedLiteral BGE = MaskedLiteral::IType("1100011", "101");
const MaskedLiteral BLTU = MaskedLiteral::IType("1100011", "110");
const MaskedLiteral BGEU = MaskedLiteral::IType("1100011", "111");

const MaskedLiteral LB = MaskedLiteral::IType("0000011", "000");
const MaskedLiteral LH = MaskedLiteral::IType("0000011", "001");
const MaskedLiteral LW = MaskedLiteral::IType("0000011", "010");
const MaskedLiteral LBU = MaskedLiteral::IType("0000011", "100");
const MaskedLiteral LHU = MaskedLiteral::IType("0000011", "101");

const MaskedLiteral SB = MaskedLiteral::IType("0100011", "000");
const MaskedLiteral SH = MaskedLiteral::IType("0100011", "001");
const MaskedLiteral SW = MaskedLiteral::IType("0100011", "010");

const MaskedLiteral ADDI = MaskedLiteral::IType("0010011", "000");
const MaskedLiteral SLTI = MaskedLiteral::IType("0010011", "010");
const MaskedLiteral SLTIU = MaskedLiteral::IType("0010011", "011");
const MaskedLiteral XORI = MaskedLiteral::IType("0010011", "100");
const MaskedLiteral ORI = MaskedLiteral::IType("0010011", "110");
const MaskedLiteral ANDI = MaskedLiteral::IType("0010011", "111");

const MaskedLiteral SLLI = MaskedLiteral::RType("0010011", "001", "0000000");
const MaskedLiteral SRLI = MaskedLiteral::RType("0010011", "101", "0000000");
const MaskedLiteral SRAI = MaskedLiteral::RType("0010011", "101", "0100000");

const MaskedLiteral ADD = MaskedLiteral::RType("0110011", "000", "0000000");
const MaskedLiteral SUB = MaskedLiteral::RType("0110011", "000", "0100000");
const MaskedLiteral SLL = MaskedLiteral::RType("0110011", "001", "0000000");
const MaskedLiteral SLT = MaskedLiteral::RType("0110011", "010", "0000000");
const MaskedLiteral SLTU = MaskedLiteral::RType("0110011", "011", "0000000");
const MaskedLiteral XOR = MaskedLiteral::RType("0110011", "100", "0000000");
const MaskedLiteral SRL = MaskedLiteral::RType("0110011", "101", "0000000");
const MaskedLiteral SRA = MaskedLiteral::RType("0110011", "101", "0100000");
const MaskedLiteral OR = MaskedLiteral::RType("0110011", "110", "0000000");
const MaskedLiteral AND = MaskedLiteral::RType("0110011", "111", "0000000");

const MaskedLiteral FENCE = MaskedLiteral::IType("0001111", "000");

const MaskedLiteral CSRRW = MaskedLiteral::IType("1110011", "001");
const MaskedLiteral CSRRS = MaskedLiteral::IType("1110011", "010");
const MaskedLiteral CSRRC = MaskedLiteral::IType("1110011", "011");
const MaskedLiteral CSRRWI = MaskedLiteral::IType("1110011", "101");
const MaskedLiteral CSRRSI = MaskedLiteral::IType("1110011", "110");
const MaskedLiteral CSRRCI = MaskedLiteral::IType("1110011", "111");

}  // namespace RV32I

namespace RV32M {
const MaskedLiteral MUL = MaskedLiteral::RType("0110011", "000", "0000001");
const MaskedLiteral MULH = MaskedLiteral::RType("0110011", "001", "0000001");
const MaskedLiteral MULHSU = MaskedLiteral::RType("0110011", "010", "0000001");
const MaskedLiteral MULHU = MaskedLiteral::RType("0110011", "011", "0000001");
const MaskedLiteral DIV = MaskedLiteral::RType("0110011", "100", "0000001");
const MaskedLiteral DIVU = MaskedLiteral::RType("0110011", "101", "0000001");
const MaskedLiteral REM = MaskedLiteral::RType("0110011", "110", "0000001");
const MaskedLiteral REMU = MaskedLiteral::RType("0110011", "111", "0000001");
}  // namespace RV32M

namespace EXTRA {
const MaskedLiteral EXIT = MaskedLiteral::UType("0001011");
}  // namespace EXTRA
