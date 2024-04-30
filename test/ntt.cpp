#define MOD 7681
#define g 17
int power(int a, int b) {
    int ret = 1;
    for (; b; b >>= 1, (a *= a) %= MOD)
        if (b & 1) (ret *= a) %= MOD;
    return ret;
}
int rev[1024];
void NTT(int *a, int len, int flag) {
    for (int i = 0; i < len; i++)
        if (rev[i] > i) {
            int tmp = a[i];
            a[i] = a[rev[i]];
            a[rev[i]] = tmp;
        }
    for (int k = 2; k <= len; k <<= 1) {
        int m = k >> 1;
        int wn =
            power(g, flag == 1 ? (MOD - 1) / k : (MOD - 1) - (MOD - 1) / k);
        for (int i = 0; i < len; i += k) {
            int w = 1;
            for (int j = 0; j < m; j++) {
                int tmp1 = a[i | j], tmp2 = a[i | j | m] * w % MOD;
                a[i | j] = (tmp1 + tmp2) % MOD;
                a[i | j | m] = (tmp1 - tmp2 + MOD) % MOD;
                (w *= wn) %= MOD;
            }
        }
    }
    if (flag == -1) {
        int rev = power(len, MOD - 2);
        for (int i = 0; i < len; i++) (a[i] *= rev) %= MOD;
    }
}
int a[16] = {1, 5, 7, 6, 4, 2, 10, 8, 0, 0, 0, 0, 0, 0, 0};
int b[16] = {2, 9, 7, 6, 10, 2, 3, 9, 0, 0, 0, 0, 0, 0, 0};
int c[16], d[16];
int flag;
const int n = 7, m = 7;
int main() {
    int tl = 1;
    while (tl <= n + m) tl <<= 1;
    int half = tl >> 1;
    for (int i = 0; i < tl; i++)
        rev[i] = (rev[i >> 1] >> 1) | ((i & 1) ? half : 0);

    for (int i = 0; i < 16; i++) c[i] = 0;
    for (int i = 0; i < 16; i++) d[i] = 0;

    for (int i = 0; i <= n; i++)
        for (int j = 0; j <= m; j++) d[i + j] += a[i] * b[j];

    NTT(a, tl, 1);
    NTT(b, tl, 1);
    for (int i = 0; i < tl; i++) c[i] = a[i] * b[i] % MOD;
    NTT(c, tl, -1);

    flag = 1;
    for (int i = 0; i < 16; i++)
        if (c[i] != d[i]) {
            flag = 0;
            break;
        }

    asm("mv t0, %[src]\n"
        ".word 0x0000000b\n"
        :
        : [src] "r"(flag)
        : "t0");

    return 0;
}
