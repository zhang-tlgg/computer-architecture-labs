#include <stdexcept>

#include "logger.h"
#include "processor.h"

/**
 * @brief 处理前端流出的指令
 *
 * @param inst 前端将要流出的指令（在流出buffer里面）
 * @return true 后端接受该指令
 * @return false 后端拒绝该指令
 */
bool Backend::dispatchInstruction([[maybe_unused]] const Instruction &inst) {
    // TODO: Check rob and reservation station is available for push.
	// 1.检查 ROB 是否已满
	// 2.检查对应的保留站是否已满
	// 3.插入 ROB
	// 4.插入保留站
	// 5.更新寄存器占用情况
	// 6.插入成功返回 true, 失败返回 false, 请保证返回 false 时没有副作用
    // NOTE: use getFUType to get instruction's target FU
    // NOTE: FUType::NONE only goes into ROB but not Reservation Stations
    if (!rob.canPush()) {
		Logger::Info("ROB is full");
		return false;
	}
	std::stringstream ss;
	ss << inst;
	Logger::Debug("[Back::Insert] %s at pc = %x", ss.str().c_str(), inst.pc);
	
	if (!rob.canPush()) {
		Logger::Info("ROB can not push.");
		return false;
	}
	switch (getFUType(inst))
	{
	case FUType::ALU: {
		if (!rsALU.hasEmptySlot()) {
			Logger::Info("rsALU is full.");
			// rsALU.showContent();
			return false;
		}
		unsigned int robIdx = rob.push(inst, false);
		rsALU.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		rob.showContent();
		return true;
	}
	case FUType::BRU: {
		if (!rsBRU.hasEmptySlot()) {
			Logger::Info("rsBRU is full.");
			return false;
		}
		unsigned int robIdx = rob.push(inst, false);
		rsBRU.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		rob.showContent();
		return true;
	}
	case FUType::DIV: {
		if (!rsDIV.hasEmptySlot()) {
			Logger::Info("rsDIV is full.");
			return false;
		}
		unsigned int robIdx = rob.push(inst, false);
		rsDIV.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		rob.showContent();
		return true;
	}
	case FUType::MUL: {
		if (!rsMUL.hasEmptySlot()) return false;
		unsigned int robIdx = rob.push(inst, false);
		rsMUL.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		rob.showContent();
		return true;
	}
	case FUType::LSU: {
		if (!rsLSU.hasEmptySlot()) {
			Logger::Info("rsLSU is full.");
			// rsLSU.showContent();
			return false;
		}
		unsigned int robIdx = rob.push(inst, false);
		rsLSU.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		rob.showContent();
		return true;
	}
	case FUType::NONE: {
		unsigned int robIdx = rob.push(inst, true);
		regFile->markBusy(inst.getRd(), robIdx);
		rob.showContent();
		return true;
	}
	
	default: {
		Logger::Error("Unknown FUType for instruction!");
		std::__throw_runtime_error("Unknown FUType for instruction!");
		return false;
		break;
	}
	}

    Logger::Error("Instruction dispatch not implemented!");
    std::__throw_runtime_error("Instruction dispatch not implemented!");
    
    return false;
}

/**
 * @brief 后端完成指令提交
 *
 * @param entry 被提交的 ROBEntry
 * @param frontend 前端，用于调用前端接口
 * @return true 提交了 EXTRA::EXIT
 * @return false 其他情况
 */
bool Backend::commitInstruction([[maybe_unused]] const ROBEntry &entry,
                                [[maybe_unused]] Frontend &frontend) {
    // TODO: Commit instructions here.
	// 1.完成指令提交
	// 2.(Optional) 更新 bpu
	// 3.ROB 弹出
	// 4.若 mispredict，跳转，清空
	// 5.若提交 Exit，返回 true，否则返回 false
    // Set executeExit when committing EXTRA::EXIT

    // NOTE: Be careful about Store Buffer!
    // NOTE: Re-executing load instructions when it is invalidated in load
    // buffer.
    // NOTE: Be careful about flush!
    // Optional TODO: Update your BTB when necessary
	std::stringstream ss;
    ss << entry.inst;
	Logger::Debug("[Back::Commit] entry %d: %s.", rob.getPopPtr(), ss.str().c_str());

	if (!entry.state.ready) {
		Logger::Error("Can not commit unready instruction");
    	std::__throw_runtime_error("Can not commit unready instruction");
	}

	if (entry.inst == EXTRA::EXIT){
		return true;
	}
	
	if (entry.state.mispredict) {
		Logger::Debug("Ready to jump!");
		if (entry.inst == RV32I::JAL || entry.inst == RV32I::JALR) {
			if (!rob.canPop()) {
				Logger::Error("Can not pop from empty ROB!");
				std::__throw_runtime_error(
				    "Can not pop from empty ROB!");
			}
			regFile->write(entry.inst.getRd(), entry.state.result, rob.getPopPtr());
			Logger::Debug("Return addr = %x, Param a0 = %d", regFile->read(1), regFile->read(10));
		}
		frontend.jump(entry.state.jumpTarget);
		flush();
		return false;
		// rob.flush();
	}

	if (rob.canPop()) {
		switch (getFUType(entry.inst))
		{
		case FUType::ALU:
		case FUType::DIV:
		case FUType::MUL:
		case FUType::BRU:
		{	
			unsigned rd = entry.inst.getRd();
			regFile->write(rd, entry.state.result, rob.getPopPtr());
			rob.pop();
			if (entry.inst.type != InstructionType::B)
				Logger::Debug("Reg[%d] := %d = 0x%x", rd, entry.state.result, entry.state.result);
			break;
		}
		case FUType::LSU:
		{	
		 	if (entry.inst == RV32I::LB || entry.inst == RV32I::LH || entry.inst == RV32I::LHU || 
				entry.inst == RV32I::LW || entry.inst == RV32I::LBU) { // Load 

                LoadBufferSlot ldSlot = loadBuffer.pop(rob.getPopPtr());
				if (ldSlot.invalidate) {
					unsigned rd = entry.inst.getRd();
					Logger::Debug("invalidate: Reg[%d] := %d = 0x%x", rd, entry.state.result, entry.state.result);

                    // Instruction inst = entry.inst;
                    // rob.pop();
					// // 是这样吗？
                    // dispatchInstruction(inst);
                }
				unsigned rd = entry.inst.getRd();
				regFile->write(rd, entry.state.result, rob.getPopPtr());
				Logger::Debug("Reg[%d] := %d = 0x%x", rd, entry.state.result, entry.state.result);
				rob.pop();
			}
			else if (entry.inst == RV32I::SB || entry.inst == RV32I::SH || entry.inst == RV32I::SW) { // Store
				StoreBufferSlot stSlot = storeBuffer.front();
				bool status =
					writeMemoryHierarchy(stSlot.storeAddress, stSlot.storeData, 0xF);
				if (!status) {
					return false;
				} else {
					storeBuffer.pop();
				}
				Logger::Debug("Mem[%x] := %d", stSlot.storeAddress, stSlot.storeData);
			}
			else {
				Logger::Error("Committing unknown instruction! FUType = LSU, InstType != Load/Store.");
				std::__throw_runtime_error(
					"Committing unknown instruction!");
			}
			break;
		}	
		case FUType::NONE:
		{
			if (entry.inst == EXTRA::EXIT){
				return true;
			}
			else {
				Logger::Error("Committing unknown instruction! FUType = NONE, InstType != Exit.");
				std::__throw_runtime_error(
					"Committing unknown instruction!");
			}
		}
		default:
		{
			Logger::Error("Committing unknown instruction! FUType unknown.");
			std::__throw_runtime_error(
				"Committing unknown instruction!");
			break;
		}
		}
		rob.showContent();
		return false;
	}
	else {
		Logger::Error("Can not commit instruction when ROB is empty!");
		std::__throw_runtime_error(
		    "Can not commit instruction when ROB is empty!");
	}
}
