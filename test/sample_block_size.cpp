constexpr unsigned ARRAY_SIZE = (1u << 20u);  // 1 MB
char arr[ARRAY_SIZE];

constexpr int totalSize = 5000;
constexpr int totalTime = 3000;

int main(int argc, char **argv) {
    auto step = ((unsigned) argv[0]);
    int sum = 0;

    for (int i = 0; i < totalTime; i++) {
        sum += arr[ i * step % totalSize];
    }

    asm volatile(".word 0x0000000b"  // exit mark
    );

    return sum;
}
