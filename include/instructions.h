#pragma once

#include <utility>

#include "branch_predict_model.h"
#include "defines.h"
#include "masked_literal.h"

struct ExecuteResultBundle {
    bool mispredict;
    bool actualTaken;
    unsigned result;
    unsigned jumpTarget;
};

struct Instruction {
    unsigned instruction;
    InstructionType type;
    unsigned pc;
    BranchPredictBundle predictBundle;

    [[nodiscard]] unsigned getImmediate() const;
    [[nodiscard]] unsigned getRd() const;
    [[nodiscard]] unsigned getRs1() const;
    [[nodiscard]] unsigned getRs2() const;
    [[nodiscard]] unsigned get(int index) const;
    [[nodiscard]] unsigned get(int index, int length) const;

    explicit Instruction(unsigned inst = 0x13);
    bool operator==(const MaskedLiteral &rhs) const;
    bool operator!=(const MaskedLiteral &rhs) const;
    friend std::ostream &operator<<(std::ostream &os, const Instruction &inst);

    [[nodiscard]] ExecuteResultBundle execute(const std::string &name,
                                              unsigned operand1,
                                              unsigned operand2) const;

    static Instruction NOP();
};

FUType getFUType(const Instruction &inst);
