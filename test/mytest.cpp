const int n = 64;
int a[64] = {
    24,  4,   228, 78,  104, 109, 124, 49,  
    187, 57,  64,  28,  129, 130, 248, 200, 
    65,  70,  133, 236, 32,  108, 33,  182, 
    195, 65,  48,  66,  48,  46, 143, 254, 
    157, 145, 101, 116, 238, 143, 151, 28,  
    87,  70,  211, 174, 246, 165, 26,  105, 
    108, 2,   20,  151, 5,   168, 189, 93,  
    188, 120, 147, 31, 50,  224, 189, 54};

int b[64];

void merge_sort(int l, int r) {
    if (l == r) return;
    int m = l + (r - l) / 2;
    merge_sort(l, m);
    merge_sort(m + 1, r);

    int i = l, j = m + 1, t = 0;
    while (i <= m && j <= r)
        if (a[i] <= a[j])
            b[t++] = a[i++];
        else
            b[t++] = a[j++];
    while (i <= m) b[t++] = a[i++];
    while (j <= r) b[t++] = a[j++];
    for (int i = 0; i < t; i++) a[l + i] = b[i];
}

int main() {
    merge_sort(0, n - 1);
    asm volatile(".word 0x0000000b"  // exit mark
    );
    return 0;
}