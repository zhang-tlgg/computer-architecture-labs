#include "cache-exp.h"
#include "processor.h"
#include "runner.h"

unsigned MeasureCacheBlockSize([[maybe_unused]] ProcessorAbstract *p) {
    // TODO: Measure the cacheline size (or block size) of the cache in the given processor
    // The size will be ranged from 4 bytes to 32 bytes, and must be a power of 2
    // Return the accurate value

    [[maybe_unused]] unsigned testSize[6] = {2, 4, 8, 16, 32, 64};

    return 4;
}
