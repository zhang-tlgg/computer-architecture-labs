#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "cache-exp.h"
#include "cxxopts.hpp"
#include "logger.h"
#include "processor.h"
#include "runner.h"
#include "with_cache.h"

[[maybe_unused]] ProcessorWithCache *processorWC = nullptr;

int main(int argc, char **argv) {
    cxxopts::Options options("tomasulo-cache-runner",
                             "Tomasulo With Cache Runner");
    auto adder = options.add_options();
    adder("o,output",
          "Output Log file",
          cxxopts::value<std::string>()->default_value("output.log"));
    adder("h,help", "Print Usage");
    adder("d,debug", "Print debug infos");
    adder("l,latency",
          "Memory Latency",
          cxxopts::value<int>()->default_value("0"));
    adder("cache-size", "Cache Size", cxxopts::value<int>());
    adder("block-size", "Cache Block Size", cxxopts::value<int>());
    adder("a,associativity", "Cache Associativity", cxxopts::value<int>());
    adder("write-through", "Cache Write Through");
    adder("replace-type", "Cache Replace Type", cxxopts::value<std::string>());
    adder("matmul", "Do Matrix Multiplication");

    auto result = options.parse(argc, argv);
    if (result.count("help") != 0 || !result.unmatched().empty() || argc == 1) {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    if (result.count("debug") != 0) {
        Logger::setInfoOutput(true);
    } else {
        Logger::setInfoOutput(false);
    }

    std::vector<unsigned> inst, data;
    auto latency = result["latency"].as<int>();

    auto cacheSize = result["cache-size"].as<int>();
    auto blockSize = result["block-size"].as<int>();
    auto associativity = result["associativity"].as<int>();
    bool writeThrough = result.count("write-through") != 0;
    auto typeString = result["replace-type"].as<std::string>();

    bool doMatMul = result.count("matmul") != 0;

    ReplaceType replaceType;
    if (typeString == "FIFO")
        replaceType = ReplaceType::FIFO;
    else if (typeString == "LRU")
        replaceType = ReplaceType::LRU;
    else
        replaceType = ReplaceType::RANDOM;

    processorWC = new ProcessorWithCache(std::vector<unsigned>(),
                                         std::vector<unsigned>(),
                                         0x80000000u,
                                         latency,
                                         cacheSize,
                                         blockSize,
                                         associativity,
                                         writeThrough,
                                         replaceType);

    int cacheSizeResult = (int) MeasureCacheSize(processorWC);

    int score = 0;

    Logger::Warn("Cache Size Test Result: %u, Actual Cache Size: %u",
                 cacheSizeResult,
                 cacheSize);

    if (cacheSizeResult != cacheSize) {
        fprintf(
            stderr,
            "[ FAILED  ] On testcase %d, answer is %d, but %d is returned\n",
            1,
            cacheSize,
            cacheSizeResult);
    } else {
        score += 20;
    }

    int cacheBlockSizeResult = (int) MeasureCacheBlockSize(processorWC);

    Logger::Warn(
        "Cache Block Size Test Result: %u, Actual Cache Block Size: %u",
        cacheBlockSizeResult,
        blockSize);

    if (cacheBlockSizeResult != blockSize) {
        fprintf(
            stderr,
            "[ FAILED  ] On testcase %d, answer is %d, but %d is returned\n",
            2,
            blockSize,
            cacheBlockSizeResult);
    } else {
        score += 20;
    }

    int cacheAssoResult =
        (int) MeasureCacheAssociativity(processorWC, cacheSize, blockSize);

    Logger::Warn(
        "Cache Associativity Test Result: %u, Actual Cache Associativity: %u",
        cacheAssoResult,
        associativity);
    
    if (cacheAssoResult != associativity) {
        fprintf(
            stderr,
            "[ FAILED  ] On testcase %d, answer is %d, but %d is returned\n",
            3,
            associativity,
            cacheAssoResult);
    } else {
        score += 30;
    }

    bool matmulOptimize = !doMatMul || MeasureMatmulWithCache(processorWC);

    if (!matmulOptimize) {
        fprintf(stderr,
                "[ FAILED  ] On testcase %d, optimization did not meet the "
                "requirement\n",
                4);
    } else {
        score += 30;
    }

    auto testReplaceType = GetCacheReplaceType(processorWC);

    if (testReplaceType == replaceType) {
        fprintf(stderr,
                "[    OK   ] You correctly measured the replacement policy of the cache\n");
        score += 10;
    }

    bool testWriteThrough = CheckCacheWriteThrough(processorWC);
    if(testWriteThrough == writeThrough) {
        fprintf(stderr,
                "[    OK   ] You correctly measured the write policy of the cache\n");
        score += 10;
    }

    score = std::min(score, 100);

    Logger::Warn("Final score: %d\n", score);

    return score;
}
