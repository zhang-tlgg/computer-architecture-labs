// baseline_matmul.cpp
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

    int BLOCK_SIZE = 16;
    for (int blk = 0; blk < MATRIX_SIZE / BLOCK_SIZE; ++blk) {
        for (int i = 0; i < MATRIX_SIZE; i ++) {
            for (int k = blk * BLOCK_SIZE; k < (blk + 1) * BLOCK_SIZE; k ++) {
                register int tmp = A[i][k];
                for (int j = 0; j < MATRIX_SIZE; j ++)
                    C[i][j] += tmp * B[k][j];
            }
        }
    }


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
