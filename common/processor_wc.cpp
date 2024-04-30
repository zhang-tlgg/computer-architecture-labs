#include "with_cache.h"

ProcessorWithCache::ProcessorWithCache(const std::vector<unsigned> &inst,
                                       const std::vector<unsigned> &data,
                                       unsigned entry,
                                       unsigned memoryLatency,
                                       unsigned cacheSize,
                                       unsigned cacheBlockSize,
                                       unsigned cacheAssociativity,
                                       bool cacheWriteThrough,
                                       ReplaceType cacheReplaceType)
    : regFile(),
      frontend(inst),
      backend(data,
              &regFile,
              memoryLatency,
              cacheSize,
              cacheBlockSize,
              cacheAssociativity,
              cacheWriteThrough,
              cacheReplaceType) {
    frontend.jump(entry);
}

/**
 * @brief Processor 步进函数
 *
 * @return true 提交了EXTRA::EXIT
 * @return false 其他情况
 */
bool ProcessorWithCache::step() {
    bool finish = backend.step(frontend);
    auto newInst = frontend.step();
    if (newInst.has_value()) {
        if (!backend.dispatchInstruction(newInst.value()))
            frontend.haltDispatch();
        else {
            std::stringstream ss;
            ss << newInst.value();
            Logger::Info("Dispatching %s with pc = %08x\n",
                         ss.str().c_str(),
                         newInst.value().pc);
        }
    }
    return finish;
}

/**
 * @brief 用于读取数据内存中的内容
 *
 * @param addr
 * @return unsigned
 */
unsigned ProcessorWithCache::readMem(unsigned addr) const {
    return backend.read(addr);
}

/**
 * @brief 用于写入数据内存
 * 
 * @param addr 
 * @param val 
 */
void ProcessorWithCache::writeMem(unsigned addr, unsigned val) {
    backend.functionalWrite(addr, val);
}

/**
 * @brief 用于读取寄存器当中的内容
 *
 * @param addr
 * @return unsigned
 */
unsigned ProcessorWithCache::readReg(unsigned addr) const {
    return regFile.read(addr);
}

/**
 * @brief 用于写入寄存器
 * 
 * @param addr 
 * @param value 
 */
void ProcessorWithCache::writeReg(unsigned addr, unsigned value) {
    regFile.functionalWrite(addr, value);
}

/**
 * @brief 让 CPU 加载指定程序
 * 
 * @param inst 
 * @param data 
 * @param entry 
 */
void ProcessorWithCache::loadProgram(const std::vector<unsigned int> &inst,
                                     const std::vector<unsigned int> &data,
                                     unsigned int entry) {
    frontend.reset(inst, entry);
    backend.reset(data);
    regFile.reset();
}
