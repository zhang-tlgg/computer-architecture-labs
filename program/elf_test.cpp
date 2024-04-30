#include <iostream>
#include <string>

#include "defines.h"
#include "instructions.h"
#include "logger.h"

int main() {
    Logger::setInfoOutput(true);

    std::string name;
    std::cout << "Elf file name: ";
    std::cin >> name;

    std::vector<unsigned> inst, data;

    readElf(name, inst, data);

    unsigned p = 0x80000000;

    for (auto &x : inst) {
        if (x != 0) {
            Instruction i(x);
            std::cout << std::hex << "0x" << p << ": " << std::dec << i
                      << std::endl;
        }
        p += 4;
    }

    std::cout << std::endl;

    p = 0x80400000;

    for (auto &x : data) {
        std::cout << std::hex << "0x" << p << ": " << x << std::dec
                  << std::endl;
        p += 4;
    }

    return 0;
}