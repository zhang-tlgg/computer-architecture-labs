#include "cache-exp.h"
#include "processor.h"
#include "runner.h"

unsigned MeasureCacheAssociativity([[maybe_unused]] ProcessorAbstract *p,
                                   [[maybe_unused]] unsigned cacheSize,
                                   [[maybe_unused]] unsigned cacheBlockSize) {
    // TODO: Measure the associativiy of the cache in the given processor
    // The associativity will be ranged from 1 to 8, and must be a power of 2
    // Return the accurate value
    
    [[maybe_unused]] unsigned testAsso[6] = {1, 2, 4, 8, 16, 32};

    return 1;
}
