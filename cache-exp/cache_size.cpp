#include "cache-exp.h"
#include "processor.h"
#include "runner.h"

/*
基本思路：从内存中连续读取数组中不同大小的数据，观察平均读取速度。
当数组大小超过数据缓存之后，会出现 L1 DCache 的读缺失，平均读取速度会增加。
*/

unsigned MeasureCacheSize([[maybe_unused]] ProcessorAbstract *p) {

    // TODO: Measure the size of the cache in the given processor
    // The size will be ranged from 512 bytes to 4096 bytes(or 4 KB), and must be a power of 2
    // Return the accurate value

    unsigned testSize[5] = {512, 1024, 2048, 4096, 8192};
    unsigned testTime[5];

    for (int i = 0; i < 5; i++) {
        testTime[i] = execute(p, "./test/sample_cache_size", 1, testSize[i]);
    }
    for (int i = 0; i < 5; i++) {
        Logger::Warn(
            "With test cache size = %u, program simulator ran %u cycles.",
            testSize[i],
            testTime[i]);
    }

    int mx = -1;
    int idx = 0;

    for (int i = 0; i < 4; i++) {
        int delta = ((int) testTime[i + 1]) - ((int) testTime[i]);
        if (delta > mx) {
            mx = delta;
            idx = i;
        }
    }

    return testSize[idx];
}
