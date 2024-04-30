#include "instructions.h"

#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "csr.h"
#include "logger.h"

/**
 * @brief Construct a new Instruction:: Instruction object
 *
 * @param inst 指令编码
 */
Instruction::Instruction(unsigned int inst) {
    instruction = inst;
    using namespace RV32I;
    using namespace RV32M;
    if (inst == LUI || inst == AUIPC || inst == EXTRA::EXIT)
        type = InstructionType::U;
    else if (inst == JAL) {
        type = InstructionType::J;
    } else if (inst == JALR) {
        type = InstructionType::I;
    } else if (inst == BEQ || inst == BNE || inst == BLT || inst == BGE ||
               inst == BLTU || inst == BGEU)
        type = InstructionType::B;
    else if (inst == LB || inst == LH || inst == LW || inst == LBU ||
             inst == LHU)
        type = InstructionType::I;
    else if (inst == SB || inst == SH || inst == SW)
        type = InstructionType::S;
    else if (inst == ADDI || inst == SLTI || inst == SLTIU || inst == XORI ||
             inst == ORI || inst == ANDI || inst == SLLI || inst == SRLI ||
             inst == SRAI)
        type = InstructionType::I;
    else if (inst == ADD || inst == SUB || inst == SLL || inst == SLT ||
             inst == SLTU || inst == XOR || inst == SRL || inst == SRA ||
             inst == OR || inst == AND)
        type = InstructionType::R;
    else if (inst == MUL || inst == MULH || inst == MULHSU || inst == MULHU ||
             inst == DIV || inst == DIVU || inst == REM || inst == REMU)
        type = InstructionType::R;
    else if (inst == CSRRW || inst == CSRRS || inst == CSRRC)
        type = InstructionType::I;
    else if (inst == CSRRWI || inst == CSRRSI || inst == CSRRCI)
        type = InstructionType::U;
    else if (inst == FENCE)
        type = InstructionType::I;
    else {
        std::stringstream ss;
        ss << inst;
        std::string tmp;
        ss >> std::hex >> tmp;
        tmp = "Instruction " + tmp + " not implemented.";
        throw std::invalid_argument(tmp.c_str());
    }
}

/**
 * @brief 根据指令类型获取指令的立即数
 *
 * @return unsigned 立即数，没有则返回0
 */
unsigned Instruction::getImmediate() const {
    bool flag = (instruction >> 31u);
    switch (type) {
    case InstructionType::R:
        return 0;
    case InstructionType::I:
        return (instruction >> 20u) | (flag ? 0xFFFFF000u : 0);
    case InstructionType::U:
        return instruction & 0xFFFFF000u;
    case InstructionType::S:
        return ((instruction >> 7u) & 0x1fu) |
               (((instruction >> 25u) & 0x7fu) << 5u) |
               (flag ? 0xFFFFF000u : 0);
    case InstructionType::B:
        return (((instruction >> 8u) & 0xfu) << 1u) |
               (((instruction >> 25u) & 0x3fu) << 5u) |
               (((instruction >> 7u) & 1u) << 11u) | (flag ? 0xFFFFF800u : 0);
    case InstructionType::J:
        return (((instruction >> 21u) & 0x3FFu) << 1u) |
               (((instruction >> 20u) & 1u) << 11u) |
               (((instruction >> 12u) & 0xFFu) << 12u) |
               (flag ? 0xFFF00000u : 0);
    }
    return 0;
}

/**
 * @brief 根据指令类型获取指令的rd域
 *
 * @return unsigned 指令有rd时返回rd，没有时返回0
 */
unsigned Instruction::getRd() const {
    if (type != InstructionType::B && type != InstructionType::S)
        return ((instruction >> 7u) & 0x1Fu);
    return 0;
}

/**
 * @brief 根据指令类型获取指令的rs1域
 *
 * @return unsigned 指令有rs1时返回rs1，没有时返回0
 */
unsigned Instruction::getRs1() const {
    if (type != InstructionType::U && type != InstructionType::J)
        return ((instruction >> 15u) & 0x1Fu);
    return 0;
}

/**
 * @brief 根据指令类型获取指令的rs2域
 *
 * @return unsigned 指令有rs2时返回rs2，没有时返回0
 */
unsigned Instruction::getRs2() const {
    if (type == InstructionType::S || type == InstructionType::R ||
        type == InstructionType::B)
        return ((instruction >> 20u) & 0x1Fu);
    return 0;
}

/**
 * @brief 用于判断指令是否为某种类型的指令
 *
 * @param rhs 指令类型
 * @return true
 * @return false
 */
bool Instruction::operator==(const MaskedLiteral &rhs) const {
    return rhs == instruction;
}

/**
 * @brief 用于判断指令是否不为某种类型的指令
 *
 * @param rhs 指令类型
 * @return true
 * @return false
 */
bool Instruction::operator!=(const MaskedLiteral &rhs) const {
    return !(*this == rhs);
}

/**
 * @brief 指令输出函数，也即你可以使用std::ostream打印Instruction
 *
 * @param os
 * @param inst
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const Instruction &inst) {
    using namespace RV32I;
    using namespace RV32M;

    const std::string &rs1 = xreg_name[inst.getRs1()];
    const std::string &rs2 = xreg_name[inst.getRs2()];
    const std::string &rd = xreg_name[inst.getRd()];
    unsigned imm = inst.getImmediate();
    unsigned shaft = imm & 31u;

    if (inst == FENCE) {
        os << "FENCE";
        return os;
    }

    if (inst == EXTRA::EXIT) {
        os << "EXIT";
        return os;
    }

    if (inst == CSRRW || inst == CSRRS || inst == CSRRC) {
        if (inst == CSRRW)
            os << "CSRRW ";
        else if (inst == CSRRS)
            os << "CSRRS ";
        else if (inst == CSRRC)
            os << "CSRRC ";
        os << rd << " ";
        unsigned id = imm & 0xFFFu;
        if (id == MEPC)
            os << "MEPC";
        else if (id == MSTATUS)
            os << "MSTATUS";
        else if (id == MIP)
            os << "MIP";
        else if (id == MIE)
            os << "MIE";
        else
            os << id;
        os << " " << rs1;
        return os;
    }

    if (inst == CSRRWI || inst == CSRRSI || inst == CSRRCI) {
        unsigned uimm = inst.get(15, 5);
        unsigned id = inst.get(20, 12);
        if (inst == CSRRWI)
            os << "CSRRWI ";
        else if (inst == CSRRSI)
            os << "CSRRSI ";
        else if (inst == CSRRCI)
            os << "CSRRCI ";
        os << rd << " ";
        if (id == MEPC)
            os << "MEPC";
        else if (id == MSTATUS)
            os << "MSTATUS";
        else if (id == MIP)
            os << "MIP";
        else if (id == MIE)
            os << "MIE";
        else
            os << id;
        os << " " << uimm;
        return os;
    }

    switch (inst.type) {
    case InstructionType::U:
    case InstructionType::J:
        if (inst == LUI)
            os << "LUI ";
        else if (inst == AUIPC)
            os << "AUIPC ";
        else if (inst == JAL)
            os << "JAL ";
        os << rd << ", " << imm;
        return os;

    case InstructionType::B:
    case InstructionType::S:
        if (inst == BEQ)
            os << "BEQ ";
        else if (inst == BNE)
            os << "BNE ";
        else if (inst == BLT)
            os << "BLT ";
        else if (inst == BGE)
            os << "BGE ";
        else if (inst == BLTU)
            os << "BLTU ";
        else if (inst == BGEU)
            os << "BGEU ";
        else if (inst == SB)
            os << "SB ";
        else if (inst == SH)
            os << "SH ";
        else if (inst == SW)
            os << "SW ";
        os << rs1 << ", " << rs2 << ", " << imm;
        return os;

    case InstructionType::I:
        if (inst == ADDI)
            os << "ADDI ";
        else if (inst == SLTI)
            os << "SLTI ";
        else if (inst == SLTIU)
            os << "SLTIU ";
        else if (inst == XORI)
            os << "XORI ";
        else if (inst == ORI)
            os << "ORI ";
        else if (inst == ANDI)
            os << "ANDI ";
        else if (inst == SLLI) {
            os << "SLLI ";
            imm = shaft;
        } else if (inst == SRLI) {
            os << "SRLI ";
            imm = shaft;
        } else if (inst == SRAI) {
            os << "SRAI ";
            imm = shaft;
        } else if (inst == JALR)
            os << "JALR ";
        else if (inst == LW)
            os << "LW ";
        else if (inst == LH)
            os << "LH ";
        else if (inst == LHU)
            os << "LHU ";
        else if (inst == LB)
            os << "LB ";
        else if (inst == LBU)
            os << "LBU ";
        else if (inst == JALR)
            os << "JALR ";
        os << rd << ", " << rs1 << ", " << imm;
        return os;

    case InstructionType::R:
        if (inst == ADD)
            os << "ADD ";
        else if (inst == SUB)
            os << "SUB ";
        else if (inst == SLL)
            os << "SLL ";
        else if (inst == SLT)
            os << "SLT ";
        else if (inst == SLTU)
            os << "SLTU ";
        else if (inst == XOR)
            os << "XOR ";
        else if (inst == SRL)
            os << "SRL ";
        else if (inst == SRA)
            os << "SRA ";
        else if (inst == OR)
            os << "OR ";
        else if (inst == AND)
            os << "AND ";
        else if (inst == MUL)
            os << "MUL ";
        else if (inst == MULH)
            os << "MULH ";
        else if (inst == MULHSU)
            os << "MULHSU ";
        else if (inst == MULHU)
            os << "MULHU ";
        else if (inst == DIV)
            os << "DIV ";
        else if (inst == DIVU)
            os << "DIVU ";
        else if (inst == REM)
            os << "REM ";
        else if (inst == REMU)
            os << "REMU ";
        os << rd << ", " << rs1 << ", " << rs2;
        return os;
    }

    std::stringstream ss;
    ss << inst.instruction;
    std::string tmp;
    ss >> std::hex >> tmp;
    tmp = "Instruction " + tmp + " not implemented.";
    throw std::invalid_argument(tmp.c_str());
    return os;
}

/**
 * @brief 返回一个空指令编码
 *
 * @return Instruction
 */
Instruction Instruction::NOP() {
    Instruction ret(0x13);
    ret.type = InstructionType::I;
    return ret;
}

/**
 * @brief 用于获取指令编码的某一位
 *
 * @param index 0下标
 * @return unsigned 0/1
 */
unsigned Instruction::get(int index) const {
    if (index >= 32 || index < 0) {
        throw std::invalid_argument("Index out of range");
    }
    return (instruction >> index) & 1u;
}

/**
 * @brief 获取指令的(index+length，index]的编码
 *
 * @param index 0下标
 * @param length 保证index+length <= 32
 * @return unsigned
 */
unsigned Instruction::get(int index, int length) const {
    if (index < 0 || index + length >= 32 || length < 0) {
        throw std::invalid_argument("Index/Length out of range");
    }
    if (length == 32) {
        return instruction;
    }
    unsigned tmp = (1u << length) - 1;
    return (instruction >> index) & tmp;
}

/**
 * @brief 获取指令的执行单元种类
 *
 * @param inst
 * @return FUType
 */
FUType getFUType(const Instruction &inst) {
    using namespace RV32I;
    using namespace RV32M;

    if (inst == JAL || inst == JALR || inst == BEQ || inst == BNE ||
        inst == BLT || inst == BGE || inst == BLTU || inst == BGEU) {
        return FUType::BRU;
    }

    if (inst == LB || inst == LH || inst == LW || inst == LBU || inst == LHU ||
        inst == SB || inst == SH || inst == SW) {
        return FUType::LSU;
    }

    if (inst == MUL || inst == MULH || inst == MULHSU || inst == MULHU) {
        return FUType::MUL;
    }

    if (inst == DIV || inst == DIVU || inst == REM || inst == REMU) {
        return FUType::DIV;
    }

    if (inst == EXTRA::EXIT) {
        return FUType::NONE;
    }

    return FUType::ALU;
}

/**
 * @brief 执行指令，获取执行结果
 *
 * @param name 执行指令的单元名称
 * @param operand1 操作数1，只需传入寄存器读取结果
 * @param operand2
 * 操作数2，只需传入寄存器读取结果，使用立即数时会在函数内自行读取处理
 * @return ExecuteResultBundle
 */
ExecuteResultBundle Instruction::execute(const std::string &name,
                                         unsigned operand1,
                                         unsigned operand2) const {
    using namespace RV32I;
    using namespace RV32M;

    const Instruction &inst = *this;

    ExecuteResultBundle ret{};
    unsigned &result = ret.result;
    bool &mispredict = ret.mispredict;
    bool &actualTaken = ret.actualTaken;
    unsigned &jumpTarget = ret.jumpTarget;

    if (name == "ALU") {
        if (inst == LUI)
            result = getImmediate();
        else if (inst == AUIPC)
            result = pc + getImmediate();
        else {
            std::function<unsigned(unsigned, unsigned)> func;
            if (inst == ADD || inst == ADDI)
                func = [](unsigned a, unsigned b) { return a + b; };
            else if (inst == SUB)
                func = [](unsigned a, unsigned b) { return a - b; };
            else if (inst == SLT || inst == SLTI)
                func = [](unsigned a, unsigned b) {
                    return (int) a < (int) b ? 1 : 0;
                };
            else if (inst == SLTU || inst == SLTIU)
                func = [](unsigned a, unsigned b) { return a < b ? 1 : 0; };
            else if (inst == SLL || inst == SLLI)
                func = [](unsigned a, unsigned b) { return a << (b & 31); };
            else if (inst == SRL || inst == SRLI)
                func = [](unsigned a, unsigned b) { return a >> (b & 31); };
            else if (inst == SRA || inst == SRAI)
                func = [](unsigned a, unsigned b) {
                    return (int) a >> ((int) b & 31);
                };
            else if (inst == AND || inst == ANDI)
                func = [](unsigned a, unsigned b) { return a & b; };
            else if (inst == OR || inst == ORI)
                func = [](unsigned a, unsigned b) { return a | b; };
            else if (inst == XOR || inst == XORI)
                func = [](unsigned a, unsigned b) { return a ^ b; };
            else {
                std::stringstream ss;
                ss << (*this);
                Logger::Error("Incorrect instruction %s executed in %s",
                              ss.str().c_str(),
                              name.c_str());
                throw std::runtime_error("Incorrect instruction executed");
            }
            result =
                func(operand1,
                     type == InstructionType::R ? operand2 : getImmediate());
        }
        mispredict = predictBundle.predictJump;
        actualTaken = false;
        return ret;
    } else if (name == "BRU") {
        bool jump = false;
        if (inst == JAL) {
            result = pc + 4;
            jumpTarget = pc + inst.getImmediate();
            jump = true;
        } else if (inst == JALR) {
            result = pc + 4;
            jumpTarget = operand1 + getImmediate();
            jump = true;
        } else if (inst == BEQ || inst == BNE || inst == BLT || inst == BGE ||
                   inst == BLTU || inst == BGEU) {
            jumpTarget = pc + getImmediate();
            if (inst == BEQ)
                jump = operand1 == operand2;
            else if (inst == BNE)
                jump = operand1 != operand2;
            else if (inst == BLT)
                jump = (int) operand1 < (int) operand2;
            else if (inst == BGE)
                jump = (int) operand1 >= (int) operand2;
            else if (inst == BLTU)
                jump = operand1 < operand2;
            else if (inst == BGEU)
                jump = operand1 >= operand2;
        } else {
            std::stringstream ss;
            ss << (*this);
            Logger::Error("Incorrect instruction %s executed in %s",
                          ss.str().c_str(),
                          name.c_str());
            throw std::runtime_error("Incorrect instruction executed");
        }
        actualTaken = jump;
        mispredict = (predictBundle.predictJump != jump) ||
                     (jump && predictBundle.predictTarget != jumpTarget);
        return ret;
    } else if (name == "MUL") {
        std::function<unsigned(unsigned, unsigned)> func;
        if (inst == MUL)
            func = [](unsigned a, unsigned b) {
                return (unsigned) ((1LLU * a * b) & 0xFFFFFFFFu);
            };
        else if (inst == MULH)
            func = [](unsigned a, unsigned b) {
                long long tmp = 1LL * (int) a * (int) b;
                return (unsigned) ((tmp >> 32u) & 0xFFFFFFFFU);
            };
        else if (inst == MULHSU)
            func = [](unsigned a, unsigned b) {
                long long tmp = 1LL * (int) a * b;
                return (unsigned) ((tmp >> 32u) & 0xFFFFFFFFU);
            };
        else if (inst == MULHU)
            func = [](unsigned a, unsigned b) {
                long long tmp = 1LLU * a * b;
                return (unsigned) ((tmp >> 32u) & 0xFFFFFFFFU);
            };
        else {
            std::stringstream ss;
            ss << (*this);
            Logger::Error("Incorrect instruction %s executed in %s",
                          ss.str().c_str(),
                          name.c_str());
            throw std::runtime_error("Incorrect instruction executed");
        }
        result = func(operand1,
                      type == InstructionType::R ? operand2 : getImmediate());
        actualTaken = false;
        mispredict = predictBundle.predictJump;
        return ret;
    } else if (name == "DIV") {
        std::function<unsigned(unsigned, unsigned)> func;
        if (inst == DIV)
            func = [](unsigned a, unsigned b) {
                return (unsigned) ((int) a / (int) b);
            };
        else if (inst == DIVU)
            func = [](unsigned a, unsigned b) { return a / b; };
        else if (inst == REM)
            func = [](unsigned a, unsigned b) {
                return (unsigned) ((int) a % (int) b);
            };
        else if (inst == REMU)
            func = [](unsigned a, unsigned b) { return a % b; };
        else {
            std::stringstream ss;
            ss << (*this);
            Logger::Error("Incorrect instruction %s executed in %s",
                          ss.str().c_str(),
                          name.c_str());
            throw std::runtime_error("Incorrect instruction executed");
        }
        result = func(operand1,
                      type == InstructionType::R ? operand2 : getImmediate());
        actualTaken = false;
        mispredict = predictBundle.predictJump;
        return ret;
    } else if (name == "LSU") {
        if (inst == LB || inst == LH || inst == LW || inst == LBU ||
            inst == LHU || inst == SB || inst == SH || inst == SW) {
            result = operand1 + getImmediate();
        } else {
            std::stringstream ss;
            ss << (*this);
            Logger::Error("Incorrect instruction %s executed in %s",
                          ss.str().c_str(),
                          name.c_str());
            throw std::runtime_error("Incorrect instruction executed");
        }
        actualTaken = false;
        mispredict = predictBundle.predictJump;
        return ret;
    }

    Logger::Error("Unknown component name %s", name.c_str());
    throw std::runtime_error("Unknown component name");

    return ret;
}