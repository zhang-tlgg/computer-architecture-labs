const int n = 16;
int a[16] = {
    32, 51, 85, 112, 154, 138, 5, 24, 46, 62, 143, 16, 126, 79, 91, 105};

int main() {
    int min, min_id;
    for (int i = 0; i < n - 1; i++) {
        min = a[i];
        min_id = i;
        for (int j = i + 1; j < n; j++) {
            if (a[j] < min) {
                min = a[j];
                min_id = j;
            }
        }
        if (min_id != i) {
            a[min_id] = a[i];
            a[i] = min;
        }
    }
    asm volatile(".word 0x0000000b"  // exit mark
    );
    return 0;
}