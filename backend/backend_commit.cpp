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
	Logger::setDebugOutput(false);
	std::stringstream ss;
	ss << inst;
	Logger::Debug("[Back::Dispatch] %s at pc = %x", ss.str().c_str(), inst.pc);
	
	if (!rob.canPush()) {
		Logger::Debug("ROB can not push.");
		return false;
	}

	FUType ftype = getFUType(inst);
	if (ftype == FUType::ALU) {
		if (!rsALU.hasEmptySlot())
			return false;
		unsigned int robIdx = rob.push(inst, false);
		rsALU.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		return true;
	}
	if (ftype == FUType::BRU) {
		if (!rsBRU.hasEmptySlot()) 
			return false;
		unsigned int robIdx = rob.push(inst, false);
		rsBRU.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		return true;
	}
	if (ftype == FUType::DIV) {
		if (!rsDIV.hasEmptySlot()) 
			return false;
		unsigned int robIdx = rob.push(inst, false);
		rsDIV.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		return true;
	}
	if (ftype == FUType::MUL) {
		if (!rsMUL.hasEmptySlot()) 
			return false;
		unsigned int robIdx = rob.push(inst, false);
		rsMUL.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		return true;
	}
	if (ftype == FUType::LSU) {
		if (!rsLSU.hasEmptySlot())
			return false;
		unsigned int robIdx = rob.push(inst, false);
		rsLSU.insertInstruction(inst, robIdx, regFile, rob);
		regFile->markBusy(inst.getRd(), robIdx);
		return true;
	}
	if (ftype == FUType::NONE) {
		unsigned int robIdx = rob.push(inst, true);
		regFile->markBusy(inst.getRd(), robIdx);
		return true;
	}
    
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
	Logger::setDebugOutput(false);
	std::stringstream ss;
    ss << entry.inst;
	// Logger::Debug("[Back::Commit] entry %d: %s.", rob.getPopPtr(), ss.str().c_str());
	// Logger::Debug("%x  %s", entry.inst.pc, ss.str().c_str());

	if (entry.inst == EXTRA::EXIT){
		Logger::Debug("Exit");
		Logger::Info("Exit");
		return true;
	}
	
	if (entry.state.mispredict) {
		// Logger::Debug("Jump to %x.", entry.state.jumpTarget);
		if (entry.inst == RV32I::JAL || entry.inst == RV32I::JALR) {
			regFile->write(entry.inst.getRd(), entry.state.result, rob.getPopPtr());
			// Logger::Debug("Return addr = %x, Param a0 = %d", regFile->read(1), regFile->read(10));
		}
		frontend.jump(entry.state.jumpTarget);
		flush();
		return false;
		// rob.flush();
	}

	if (rob.canPop()) {
		FUType ftype = getFUType(inst);
		if (ftype == FUType::ALU || ftype == FUType::DIV || ftype == FUType::MUL || ftype == FUType::BRU)
		{	
			unsigned rd = entry.inst.getRd();
			regFile->write(rd, entry.state.result, rob.getPopPtr());
			rob.pop();
		}
		else if (ftype == FUType::LSU)
		{	
			if (entry.inst == RV32I::SB || entry.inst == RV32I::SH || entry.inst == RV32I::SW) { 
				// Store
                StoreBufferSlot stSlot = storeBuffer.front();
                bool status = writeMemoryHierarchy(stSlot.storeAddress, stSlot.storeData, 0xF);
                if (!status) {
                    return false;
                } else {
					if (stSlot.storeAddress >= 0x80400000 && stSlot.storeAddress <= 0x804001a8){
					}
                    storeBuffer.pop();
					rob.pop();
                }
			}
			else if (entry.inst == RV32I::LB || entry.inst == RV32I::LH || entry.inst == RV32I::LHU || 
				entry.inst == RV32I::LW || entry.inst == RV32I::LBU) { 
				// Load 
                LoadBufferSlot ldSlot = loadBuffer.pop(rob.getPopPtr());
                if (ldSlot.invalidate) {
					// Logger::Debug("load invalidate: jump to %x", entry.inst.pc);
					frontend.jump(entry.inst.pc);
					flush();
                }
                else {
                    unsigned rd = entry.inst.getRd();
                    regFile->write(rd, entry.state.result, rob.getPopPtr());
                    rob.pop();
                }
			}
		}
		else if(ftype == FUType::NONE)
		{
			if (entry.inst == EXTRA::EXIT){
				return true;
			}
		}
		return false;
	}
}
