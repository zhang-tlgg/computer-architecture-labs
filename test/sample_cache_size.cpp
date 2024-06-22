constexpr unsigned ARRAY_SIZE = (1u << 20u);  // 1 MB
char arr[ARRAY_SIZE];

constexpr int totalTime = 32;
constexpr int totalRound = 64;

int main(int argc, char **argv) {
    auto testSize = ((unsigned) argv[0]);
    int sum = 0;
    unsigned step = testSize / totalTime;

    for (int i = 0; i < totalRound; i++)
        for (unsigned j = 0; j < testSize; j += step) sum += arr[j];

    asm volatile(".word 0x0000000b"  // exit mark
    );

    return sum;
}
