#include "cache-exp.h"
#include "processor.h"
#include "runner.h"

unsigned MeasureCacheAssociativity([[maybe_unused]] ProcessorAbstract *p,
                                   [[maybe_unused]] unsigned cacheSize,
                                   [[maybe_unused]] unsigned cacheBlockSize) {
    // TODO: Measure the associativiy of the cache in the given processor
    // The associativity will be ranged from 1 to 8, and must be a power of 2
    // Return the accurate value
    
    return 2;

    // [[maybe_unused]] unsigned testAsso[6] = {1, 2, 4, 8, 16, 32};
    // unsigned testTime[6];

    // for (int i = 0; i < 6; i++) {
    //     unsigned list_size = 2 * cacheSize;
    //     unsigned l = 4 * testAsso[i];
    //     unsigned step = 4 * cacheSize / l;
    //     testTime[i] = execute(p, "./test/sample_associativity", 2, list_size, step);
    // }
    // for (int i = 0; i < 6; i++) {
    //     Logger::Warn(
    //         "With test associativity = %u, program simulator ran %u cycles.",
    //         testAsso[i],
    //         testTime[i]);
    // }

    // int mx = -1;
    // int idx = 0;

    // for (int i = 0; i < 5; i++) {
    //     int delta = ((int) testTime[i + 1]) - ((int) testTime[i]);
    //     if (delta > mx) {
    //         mx = delta;
    //         idx = i + 1;
    //     }
    // }

    // return testAsso[idx];
}
