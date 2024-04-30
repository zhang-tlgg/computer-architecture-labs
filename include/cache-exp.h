#pragma once

#include "processor.h"
#include "with_cache.h"

unsigned MeasureCacheSize(ProcessorAbstract *p);
unsigned MeasureCacheBlockSize(ProcessorAbstract *p);
unsigned MeasureCacheAssociativity(ProcessorAbstract *p,
                                   unsigned cacheSize,
                                   unsigned cacheBlockSize);
ReplaceType GetCacheReplaceType(ProcessorAbstract *p);
bool CheckCacheWriteThrough(ProcessorAbstract *p);

bool MeasureMatmulWithCache(ProcessorWithCache *p);
