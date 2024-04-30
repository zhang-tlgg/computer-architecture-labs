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
    adder("f,file", "Input check file", cxxopts::value<std::string>());
    // adder("s,script-file", "Script file", cxxopts::value<std::string>());
    adder("d,debug", "Print debug infos");

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

    auto inputFile = result["file"].as<std::string>();

    int T;
    FILE *input = fopen(inputFile.c_str(), "r");

    fscanf(input, "%d", &T);

    bool sizeOK = true, blockOK = true, assocOK = true, matmulOK = true;
    bool replOK = true, writeOK = true;

    std::vector<std::vector<bool>> results(6);

    for (int _ = 0; _ < T; _++) {
        std::vector<unsigned> inst, data;
        int latency, cacheSize, blockSize, associativity;

        fscanf(input,
               "%d %d %d %d",
               &latency,
               &cacheSize,
               &blockSize,
               &associativity);

        int buffer;
        fscanf(input, "%d", &buffer);
        auto replaceType = buffer == 0 ? ReplaceType::FIFO : ReplaceType::LRU;

        fscanf(input, "%d", &buffer);
        bool writeThrough = buffer != 0;

        fscanf(input, "%d", &buffer);
        bool doMatMul = buffer != 0;

        processorWC = new ProcessorWithCache(std::vector<unsigned>(),
                                             std::vector<unsigned>(),
                                             0x80000000u,
                                             latency,
                                             cacheSize,
                                             blockSize,
                                             associativity,
                                             writeThrough,
                                             replaceType);

        int cacheSizeResult = sizeOK ? (int) MeasureCacheSize(processorWC) : 0;

        Logger::Warn("Cache Size Test Result: %u, Actual Cache Size: %u",
                     cacheSizeResult,
                     cacheSize);

        if (cacheSizeResult != cacheSize) {
            fprintf(stderr,
                    "[ FAILED  ] On testcase %d, answer is %d, but %d is "
                    "returned\n",
                    _,
                    cacheSize,
                    cacheSizeResult);
            sizeOK = false;
        } else {
            results[0].push_back(true);
        }

        int cacheBlockSizeResult =
            blockOK ? (int) MeasureCacheBlockSize(processorWC) : 0;

        Logger::Warn(
            "Cache Block Size Test Result: %u, Actual Cache Block Size: %u",
            cacheBlockSizeResult,
            blockSize);

        if (cacheBlockSizeResult != blockSize) {
            fprintf(stderr,
                    "[ FAILED  ] On testcase %d, answer is %d, but %d is "
                    "returned\n",
                    _,
                    blockSize,
                    cacheBlockSizeResult);
            blockOK = false;
        } else {
            results[1].push_back(true);
        }

        int cacheAssoResult = assocOK ? (int) MeasureCacheAssociativity(
                                            processorWC, cacheSize, blockSize)
                                      : 0;
        Logger::Warn(
            "Cache Associativity Test Result: %u, Actual Cache Associativity: "
            "%u",
            cacheAssoResult,
            associativity);

        if (cacheAssoResult != associativity) {
            fprintf(stderr,
                    "[ FAILED  ] On testcase %d, answer is %d, but %d is "
                    "returned\n",
                    _,
                    associativity,
                    cacheAssoResult);
            assocOK = false;
        } else {
            results[2].push_back(true);
        }

        bool matmulOptimize =
            !doMatMul || (matmulOK && MeasureMatmulWithCache(processorWC));

        if (!matmulOptimize) {
            fprintf(stderr,
                    "[ FAILED  ] On testcase %d, optimization did not meet the "
                    "requirement\n",
                    _);
            matmulOK = false;
        } else {
            results[3].push_back(doMatMul);
        }

        auto testReplaceType = GetCacheReplaceType(processorWC);

        if (testReplaceType != replaceType) {
            fprintf(
                stderr,
                "[ FAILED  ] On testcase %d, incorrect replace type returned\n",
                _);
            replOK = false;
        } else {
            results[4].push_back(true);
        }

        bool testWriteThrough = CheckCacheWriteThrough(processorWC);
        if (testWriteThrough != writeThrough) {
            fprintf(
                stderr,
                "[ FAILED  ] On testcase %d, incorrect write policy returned\n",
                _);
            writeOK = false;
        } else {
            results[5].push_back(true);
        }
    }

    int score = 0;
    if (sizeOK) score += 20;
    if (blockOK) score += 30;
    if (assocOK) score += 30;
    if (matmulOK) score += 20;

    if (replOK) score += 10;
    if (writeOK) score += 10;

    fprintf(stderr, "Testcase");
    for (int i = 0; i < T; i++) fprintf(stderr, "%8d", i);
    fprintf(stderr, "\n");

    for (int i = 0; i < 6; i++) {
        if (i == 0) fprintf(stderr, "Size    ");
        else if(i == 1) fprintf(stderr, "Block   ");
        else if(i == 2) fprintf(stderr, "Assoc   ");
        else if(i == 3) fprintf(stderr, "Matmul  ");
        else if(i == 4) fprintf(stderr, "Replace ");
        else if(i == 5) fprintf(stderr, "Write   ");
        for (int j = 0; j < T; j++) {
            if ((unsigned) j < results[i].size())
                fprintf(stderr, "%s", results[i][j] ? "  PASSED" : " SKIPPED");
            else if((unsigned)j == results[i].size())
                fprintf(stderr, "  FAILED");
            else
                fprintf(stderr, " SKIPPED");
        }
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "Final score: %d\n", score);
    return 0;
}
