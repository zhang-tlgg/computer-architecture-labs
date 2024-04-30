#pragma once
#include <string>

class MaskedLiteral {
    std::string literal;

public:
    explicit MaskedLiteral(std::string s);
    bool operator==(const std::string &s) const;
    bool operator==(const unsigned &x) const;

    static MaskedLiteral UType(std::string opcode);
    static MaskedLiteral IType(std::string opcode, std::string funct3);
    static MaskedLiteral RType(std::string opcode,
                               std::string funct3,
                               std::string funct7);
    friend bool operator==(const unsigned &lhs, const MaskedLiteral &rhs);
};

namespace RV32I {
extern const MaskedLiteral LUI, AUIPC;
extern const MaskedLiteral JAL, JALR;
extern const MaskedLiteral BEQ, BNE, BLT, BGE, BLTU, BGEU;
extern const MaskedLiteral LB, LH, LW, LBU, LHU;
extern const MaskedLiteral SB, SH, SW;
extern const MaskedLiteral ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI;
extern const MaskedLiteral ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND;

extern const MaskedLiteral FENCE;
extern const MaskedLiteral ECALL, EBREAK;
extern const MaskedLiteral CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI;
}  // namespace RV32I

namespace RV32M {
extern const MaskedLiteral MUL, MULH, MULHSU, MULHU;
extern const MaskedLiteral DIV, DIVU, REM, REMU;
}  // namespace RV32M

namespace EXTRA {
extern const MaskedLiteral EXIT;
}  // namespace EXTRA
