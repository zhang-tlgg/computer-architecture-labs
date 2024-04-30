#include <string>

#include "processor.h"
#include "with_cache.h"

enum class ProcessorType { normal, predict, cache };

unsigned execute(ProcessorAbstract *p, const std::string &name, int argc, ...);
unsigned executeWithCache(ProcessorWithCache *p,
                          unsigned long &totalMemoryTime,
                          unsigned long &totalCacheHitTime,
                          const std::string &name,
                          int argc,
                          ...);
