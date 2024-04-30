constexpr unsigned MATRIX_SIZE = 16u;

unsigned A[MATRIX_SIZE][MATRIX_SIZE];
unsigned B[MATRIX_SIZE][MATRIX_SIZE];
unsigned C[MATRIX_SIZE][MATRIX_SIZE];

int main(int argc, char **argv) {
    // initialization
    for (unsigned i = 0; i < MATRIX_SIZE; ++i) {
        for (unsigned j = 0; j < MATRIX_SIZE; ++j) {
            A[i][j] = i % (j + 1);
            B[i][j] = i / (j + 1);
            C[i][j] = 0;
        }
    }

    for (int i = 0; i < MATRIX_SIZE; i++)
        for (int j = 0; j < MATRIX_SIZE; j++)
            for (int k = 0; k < MATRIX_SIZE; k++) C[i][j] += A[i][k] * B[k][j];

    unsigned sum = 0;
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            sum += C[i][j];
        }
    }

    asm volatile(".word 0x0000000b"  // exit mark
    );

    return sum;
}
