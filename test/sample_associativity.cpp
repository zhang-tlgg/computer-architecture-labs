constexpr unsigned ARRAY_SIZE = (1u << 20u);  // 1 MB
char arr[ARRAY_SIZE];

constexpr int totalTime = 2000;

int main(int argc, char **argv) {
    auto totalSize = ((unsigned) argv[0]);
    auto step = ((unsigned) argv[1]);
    int sum = 0;

    for (int i = 0; i < totalTime; i++) {
        sum += arr[ i * step % totalSize];
    }

    asm volatile(".word 0x0000000b"  // exit mark
    );

    return sum;
}
