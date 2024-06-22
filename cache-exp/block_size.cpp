#include "cache-exp.h"
#include "processor.h"
#include "runner.h"

unsigned MeasureCacheBlockSize([[maybe_unused]] ProcessorAbstract *p) {
    // TODO: Measure the cacheline size (or block size) of the cache in the given processor
    // The size will be ranged from 4 bytes to 32 bytes, and must be a power of 2
    // Return the accurate value

    return 2;

    // unsigned testSize[6] = {2, 4, 8, 16, 32, 64};
    // unsigned testTime[6];

    // for (int i = 0; i < 6; i++) {
    //     testTime[i] = execute(p, "./test/sample_block_size", 1, testSize[i]);
    // }
    // for (int i = 0; i < 6; i++) {
    //     Logger::Warn(
    //         "With test block size = %u, program simulator ran %u cycles.",
    //         testSize[i],
    //         testTime[i]);
    // }

    // int mx = -1;
    // int idx = 0;

    // for (int i = 0; i < 5; i++) {
    //     int delta = ((int) testTime[i + 1]) - ((int) testTime[i]);
    //     if (delta > mx) {
    //         mx = delta;
    //         idx = i;
    //     }
    // }

    return testSize[idx];
}
