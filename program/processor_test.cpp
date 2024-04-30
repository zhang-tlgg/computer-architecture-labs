#include "processor.h"

#include <iostream>

#include "logger.h"

[[maybe_unused]] Processor *processor = nullptr;

int main() {
    std::string name;
    std::cout << "Elf file name: ";
    std::cin >> name;

    Logger::setInfoOutput(true);
    Logger::setWarnOutput(true);

    std::vector<unsigned> inst, data;

    unsigned entry = readElf(name, inst, data);

    processor = new Processor(inst, data, entry, 4);

    bool finish = false;
    do {
        finish = processor->step();
    } while (!finish);

    for (int p = 0; p < 64; p += 4) {
        printf(
            "%08x: %u\n", 0x80400000 + p, processor->readMem(0x80400000 + p));
    }

    return 0;
}
