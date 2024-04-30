#include "cache-exp.h"

ReplaceType GetCacheReplaceType([[maybe_unused]] ProcessorAbstract *p) {
    // Optional TODO: Test the replacement policy of processor
    // The replacement policy will be either LRU or FIFO
    return ReplaceType::RANDOM;
}
