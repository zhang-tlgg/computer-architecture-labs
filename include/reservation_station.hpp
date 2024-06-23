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
        // 1.若有 slot 空闲，执行插入
        // 2.插入时，设置两个寄存器读取端口是否已经唤醒，以及对应值，必要时从 ROB 中读取。
        // 3.寄存器设置 busy
        // 4.slot busy，返回
		Logger::setDebugOutput(false);
        std::stringstream ss;
		ss << inst;
		slot.inst = inst;
    	slot.busy = true;
    	slot.robIdx = robIdx;
		Logger::Debug("[RS::Insert] %s, robIdx = %d", ss.str().c_str(), robIdx);
		unsigned rs1 = inst.getRs1();
		unsigned robIdx1 = regFile->getBusyIndex(rs1);
		if (regFile->isBusy(rs1)) {
			if (reorderBuffer.checkReady(robIdx1)) {
				slot.readPort1.waitForWakeup = false;
				slot.readPort1.robIdx = 0;
				slot.readPort1.value = reorderBuffer.read(robIdx1);
			}
			else {
				slot.readPort1.waitForWakeup = true;
				slot.readPort1.robIdx = robIdx1;
				slot.readPort1.value = 0;
			}		
		}
		else {
			slot.readPort1.waitForWakeup = false;
			slot.readPort1.robIdx = 0;
			slot.readPort1.value = regFile->read(rs1);
		}
		unsigned rs2 = inst.getRs2();
		unsigned robIdx2 = regFile->getBusyIndex(rs2);
		if (regFile->isBusy(rs2)) {
			if (reorderBuffer.checkReady(robIdx2)) {
				slot.readPort2.waitForWakeup = false;
				slot.readPort2.robIdx = 0;
				slot.readPort2.value = reorderBuffer.read(robIdx2);
			}
			else {
				slot.readPort2.waitForWakeup = true;
				slot.readPort2.robIdx = robIdx2;
				slot.readPort2.value = 0;
			}
		}
		else {
			slot.readPort2.waitForWakeup = false;
			slot.readPort2.robIdx = 0;
			slot.readPort2.value = regFile->read(rs2);
		}
		return;
    }
}

template <unsigned size>
void ReservationStation<size>::wakeup(
    [[maybe_unused]] const ROBStatusWritePort &x) {
    // TODO: Wakeup instructions according to ROB Write
    // 1.查看每个 slot 的寄存器读取端口是否已经唤醒
    // 2.如果未唤醒，比对信息，尝试唤醒指令
	Logger::setDebugOutput(false);
    std::stringstream ss;
    Logger::Info("wake up from robIdx = %s", x.robIdx);
	for (auto &slot : buffer) {
		if (slot.busy) {
			if (slot.readPort1.waitForWakeup 
			 && x.robIdx == slot.readPort1.robIdx) {
				slot.readPort1.value = x.result;
				slot.readPort1.waitForWakeup = false;
				ss << slot.inst;
				Logger::Debug("RS wake up rs1 of %s", ss.str().c_str());
			}
			if (slot.readPort2.waitForWakeup 
			 && x.robIdx == slot.readPort2.robIdx) {
				slot.readPort2.value = x.result;
				slot.readPort2.waitForWakeup = false;
				ss << slot.inst;
				Logger::Debug("RS wake up rs2 of %s", ss.str().c_str());
			}
		}
	}

}

template <unsigned size>
bool ReservationStation<size>::canIssue() const {
    // TODO: Decide whether an issueSlot is ready to issue.
    // 检查是否有已经唤醒的指令
    // 注意 Store 指令需要按序发射
    // Warning: Store instructions must be issued in order!!
    bool is_LSU = buffer[0].busy && getFUType(buffer[0].inst) == FUType::LSU;
	if(is_LSU) {
		bool first_store = false;
		for (auto &slot : buffer) {
			Instruction inst = slot.inst;
			bool is_load = inst == RV32I::LB || inst == RV32I::LH || inst == RV32I::LHU || inst == RV32I::LW || inst == RV32I::LBU;
			if (is_load) {
				if (slot.busy && !slot.readPort1.waitForWakeup && !slot.readPort2.waitForWakeup) {
					return true;
				}
			}
			else {
				if (first_store == false) {
					first_store = true;
					if (slot.busy && !slot.readPort1.waitForWakeup && !slot.readPort2.waitForWakeup) {
						return true;
					}
				}
			}
		}
	}
	else {
		for (auto &slot : buffer) {
			if (slot.busy && !slot.readPort1.waitForWakeup && !slot.readPort2.waitForWakeup) {
				return true;
			}
		}
	}
	return false;
}

template <unsigned size>
IssueSlot ReservationStation<size>::issue() {
    // TODO: Issue a ready issue slot and remove it from reservation station.
    // 弹出一条完成唤醒的指令，返回发射槽
    // 你需要返回一条 busy 的发射槽！
    // 注意 Store 指令需要按序发射
    // Warning: Store instructions must be issued in order!!
    bool is_LSU = buffer[0].busy && getFUType(buffer[0].inst) == FUType::LSU;
	if(is_LSU) {
		bool first_store = false;
		IssueSlot entry;
		unsigned k = 0;
		for (k = 0; k < size; k++) {
			IssueSlot slot = buffer[k];
			Instruction inst = slot.inst;
			bool is_load = inst == RV32I::LB || inst == RV32I::LH || inst == RV32I::LHU || inst == RV32I::LW || inst == RV32I::LBU;
			if (is_load) {
				if (slot.busy && !slot.readPort1.waitForWakeup && !slot.readPort2.waitForWakeup) {
					entry = slot;
					slot.busy = false;
					break;
				}
			}
			else {
				if (first_store == false) {
					first_store = true;
					if (slot.busy && !slot.readPort1.waitForWakeup && !slot.readPort2.waitForWakeup) {
						entry = slot;
						slot.busy = false;
						break; 
					}
				}
			}
		}
		for (unsigned i = k; i < size - 1; i++) {
			buffer[i] = buffer[i+1];
		}
		buffer[size - 1] = entry;
		buffer[size - 1].busy = false;
		return entry;
	}
	else {
		IssueSlot entry;
		unsigned k = 0;
		for (k = 0; k < size; k++) {
			IssueSlot slot = buffer[k];
			if (slot.busy && !slot.readPort1.waitForWakeup && !slot.readPort2.waitForWakeup) {
				entry = slot;
				slot.busy = false;
				break;
			}
		}
		for (unsigned i = k; i < size - 1; i++) {
			buffer[i] = buffer[i+1];
		}
		buffer[size - 1] = entry;
		buffer[size - 1].busy = false;
		return entry;
	}
}

template <unsigned size>
void ReservationStation<size>::flush() {
    for (auto &slot : buffer) {
        slot.busy = false;
    }
}
