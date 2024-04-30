#include <algorithm>
#include <sstream>

#include "processor.h"
#include "rob.h"

/**
 * @brief Construct a new Backend:: Backend object
 * 用于构造后端流水线，同时初始化数据内存
 * @param rf 寄存器堆指针
 * @param data 数据内存初始化数组，从0x80400000开始
 */
Backend::Backend(const std::vector<unsigned> &data,
                 RegisterFile *const reg,
                 unsigned memoryLatency)
    : alu("ALU"),
      bru("BRU"),
      lsu("LSU"),
      mul("MUL"),
      div("DIV"),
      regFile(reg),
      memory(memoryLatency) {
    memory.functionalWrite(0, data);
}

/**
 * @brief 执行某流水线后，通过cdb唤醒其他保留站中的指令
 *
 * @param pipeline 被执行的流水线
 */
std::optional<ROBStatusWritePort> Backend::execute(ExecutePipeline &pipeline) {
    auto tmp = pipeline.step(memory, loadBuffer, rob, storeBuffer);
    return tmp;
}

/**
 * @brief 后端执行函数
 *
 * @param frontend cpu的前端，用于调用前端接口
 * @return true 提交了EXIT指令
 * @return false 未提交EXIT指令
 */
bool Backend::step(Frontend &frontend) {
    auto commit = rob.getFront();

    std::vector<std::optional<ROBStatusWritePort>> writeSave;

    writeSave.push_back(execute(alu));
    writeSave.push_back(execute(bru));
    writeSave.push_back(execute(mul));
    writeSave.push_back(execute(div));
    writeSave.push_back(execute(lsu));

    if (rsALU.canIssue() && alu.canExecute()) alu.execute(rsALU.issue());
    if (rsBRU.canIssue() && bru.canExecute()) bru.execute(rsBRU.issue());
    if (rsMUL.canIssue() && mul.canExecute()) mul.execute(rsMUL.issue());
    if (rsDIV.canIssue() && div.canExecute()) div.execute(rsDIV.issue());
    if (rsLSU.canIssue() && lsu.canExecute()) lsu.execute(rsLSU.issue());

    std::for_each(writeSave.begin(),
                  writeSave.end(),
                  [this](std::optional<ROBStatusWritePort> &tmp) {
                      if (tmp.has_value()) {
                          rsALU.wakeup(tmp.value());
                          rsBRU.wakeup(tmp.value());
                          rsMUL.wakeup(tmp.value());
                          rsDIV.wakeup(tmp.value());
                          rsLSU.wakeup(tmp.value());
                          rob.writeState(tmp.value());
                      }
                  });

    bool executeExit = false;

    if (commit.has_value() && commit.value().state.ready) {
        auto &entry = commit.value();
        executeExit = commitInstruction(entry, frontend);
    }
    return executeExit;
}

/**
 * @brief 用于刷新后端状态
 * 清空所有正在后端执行的指令数据，以及寄存器的busy标记
 *
 */
void Backend::flush() {
    rsALU.flush();
    rsBRU.flush();
    rsMUL.flush();
    rsDIV.flush();
    rsLSU.flush();

    alu.flush();
    bru.flush();
    lsu.flush();
    mul.flush();
    div.flush();

    regFile->flush();
    storeBuffer.flush();
    rob.flush();

    memory.resetState();
}

/**
 * @brief 用于读取数据内存中的数据
 *
 * @param addr 大于等于0x80400000的地址
 * @return unsigned 地址对应的数据
 */
unsigned Backend::read(unsigned addr) const {
    return memory.functionalRead((addr - 0x80400000u) >> 2u, 1)[0];
}

void Backend::functionalWrite(unsigned addr, unsigned value) {
    memory.functionalWrite((addr - 0x80400000u) >> 2u,
                           std::vector<unsigned int>({value}));
}

/**
 * @brief 写入存储结构
 * 
 * @param address 地址，0x80400000 - 0x807fffff
 * @param data 数据，按照 4 字节对齐
 * @param byteEnable 字节使能
 * @return true 写入完成
 * @return false 写入未完成
 */
bool Backend::writeMemoryHierarchy(unsigned address,
                                   unsigned data,
                                   unsigned byteEnable) {
    return memory.write((address - 0x80400000u) >> 2u, data, byteEnable);
}

void Backend::reset(const std::vector<unsigned> &data) {
    memory.functionalWrite(0, data);
    flush();
}
