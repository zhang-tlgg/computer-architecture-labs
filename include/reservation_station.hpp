#pragma once
#include <algorithm>
#include <memory>
#include <sstream>

#include "instructions.h"
#include "issue_slot.h"
#include "logger.h"
#include "register_file.h"
#include "rob.h"

template <unsigned size>
class ReservationStation {
    IssueSlot buffer[size];

public:
    ReservationStation();
    [[nodiscard]] bool hasEmptySlot() const;
    void insertInstruction(const Instruction &inst,
                           unsigned robIdx,
                           RegisterFile *regFile,
                           const ReorderBuffer &reorderBuffer);
    void wakeup(const ROBStatusWritePort &x);
    [[nodiscard]] bool canIssue() const;
    IssueSlot issue();
    void flush();
};

template <unsigned size>
ReservationStation<size>::ReservationStation() {
    for (auto &slot : buffer) {
        slot.busy = false;
    }
}

template <unsigned size>
bool ReservationStation<size>::hasEmptySlot() const {
    return std::any_of(buffer, buffer + size, [](const IssueSlot &slot) {
        return !slot.busy;
    });
}

template <unsigned size>
void ReservationStation<size>::insertInstruction(
    [[maybe_unused]] const Instruction &inst,
    [[maybe_unused]] unsigned robIdx,
    [[maybe_unused]] RegisterFile *const regFile,
    [[maybe_unused]] const ReorderBuffer &reorderBuffer) {
    for (auto &slot : buffer) {
        if (slot.busy) continue;
        // TODO: Dispatch instruction to this slot
        Logger::Error("Dispatching instruction is not implemented");
        std::__throw_runtime_error(
            "Dispatching instruction is not implemented");
    }
    Logger::Error("ReservationStation::insertInstruction no empty slots!");
    std::__throw_runtime_error(
        "No empty slots when inserting instruction into reservation "
        "station.");
}

template <unsigned size>
void ReservationStation<size>::wakeup(
    [[maybe_unused]] const ROBStatusWritePort &x) {
    // TODO: Wakeup instructions according to ROB Write
    Logger::Error("Wakeup not implemented");
    std::__throw_runtime_error("Wakeup not implemented");
}

template <unsigned size>
bool ReservationStation<size>::canIssue() const {
    // TODO: Decide whether an issueSlot is ready to issue.
    // Warning: Store instructions must be issued in order!!
    return false;
}

template <unsigned size>
IssueSlot ReservationStation<size>::issue() {
    // TODO: Issue a ready issue slot and remove it from reservation station.
    // Warning: Store instructions must be issued in order!!
    Logger::Error("No available slots for issuing");
    std::__throw_runtime_error("No available slots for issuing");
}

template <unsigned size>
void ReservationStation<size>::flush() {
    for (auto &slot : buffer) {
        slot.busy = false;
    }
}