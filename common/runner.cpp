#include "runner.h"

#include <cstdarg>

#include "defines.h"
#include "logger.h"
#include "processor.h"
#include "with_cache.h"

/**
 * @brief 让 CPU 执行指定 elf，并传入参数，参数只能是 int 或 unsigned 类型
 * 如果你希望传递其他类型参数，请联系助教。
 * 参数放置在 0x807fff00 开始的连续地址中，可以在用户程序中读取和使用
 * 注意参数数量不要过多，导致执行栈被覆盖。
 * 
 * @param p CPU
 * @param name elf 路径
 * @param argc 参数数量
 * @param ... 4字节整型参数
 * @return unsigned CPU 以该参数执行 elf 使用的时钟周期数
 */
unsigned execute(ProcessorAbstract *p, const std::string &name, int argc, ...) {
    va_list args;
    va_start(args, argc);

    std::vector<unsigned> inst, data;

    unsigned entry = readElf(name, inst, data);

    p->loadProgram(inst, data, entry);
    p->writeReg(11, 0x807fff00);

    Logger::Warn("Running %s with following arguments: ", name.c_str());

    for (int i = 0; i < argc; i++) {
        int x = va_arg(args, int);
        fprintf(stderr, "%d%c", x, " \n"[i == argc - 1]);
        p->writeMem(0x807fff00 + (i << 2u), x);
    }

    unsigned counter = 0;

    bool finish = false;
    do {
        finish = p->step();
        counter++;
        if (counter % 50000 == 0) {
            Logger::Warn("Running %u cycles.", counter);
        }
    } while (!finish);

    return counter;
}

unsigned executeWithCache(ProcessorWithCache *p,
                          unsigned long &totalMemoryTime,
                          unsigned long &totalCacheHitTime,
                          const std::string &name,
                          int argc,
                          ...) {
    va_list args;
    va_start(args, argc);

    std::vector<unsigned> inst, data;

    unsigned entry = readElf(name, inst, data);

    p->loadProgram(inst, data, entry);
    p->writeReg(11, 0x807fff00);

    Logger::Warn("Running %s with following arguments: ", name.c_str());

    for (int i = 0; i < argc; i++) {
        int x = va_arg(args, int);
        fprintf(stderr, "%d%c", x, " \n"[i == argc - 1]);
        p->writeMem(0x807fff00 + (i << 2u), x);
    }

    unsigned counter = 0;

    bool finish = false;
    do {
        finish = p->step();
        counter++;
        if (counter % 50000 == 0) {
            Logger::Warn("Running %u cycles.", counter);
        }
    } while (!finish);

    totalMemoryTime = p->getTotalMemoryTime();
    totalCacheHitTime = p->getTotalCacheHitTime();

    return counter;
}
