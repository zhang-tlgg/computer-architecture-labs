constexpr unsigned ARRAY_SIZE = (1u << 20u);  // 1 MB
char arr[ARRAY_SIZE];

constexpr int totalTime = 5000;

int main(int argc, char **argv) {
    auto testSize = ((unsigned) argv[0]);
    int sum = 0;
    unsigned step = testSize / totalTime;

    for (int i = 0; i < totalTime; i++) {
        sum += arr[ i * step % testSize];
    }

    asm volatile(".word 0x0000000b"  // exit mark
    );

    return sum;
}