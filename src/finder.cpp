#include <iostream>
#include <vector>
#include <atomic>

namespace finder
{
    const long long _worldSeed = -8978430309572744422L;

        bool isSlimeChunk(int x, int z) {
        //chequear long vs long long para extend biomes
        // new Random(seed + (long) (i * i * 4987142) + (long) (i * 5947611) + (long) (j * j) * 4392871L + (long) (j * 389711) ^ 987234911L).nextInt(10) == 0
        long seed = ((_worldSeed + (long)(x * x * 4987142) + (long)(x * 5947611) + (long)(z * z) * 4392871LL + (long)(z * 389711) ^ 987234911LL) ^ 0x5DEECE66DLL) & ((1LL << 48) - 1);
        int bits, val;
        do {
            seed = (seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
            bits = (int)(seed >> 17);
            val = bits % 10;
        } while (bits - val + 9 < 0);
        return val == 0;
    }
}

int main() {
    int x = -11, z = -7;
    if (finder::isSlimeChunk(x, z)) {
        std::cout << "(" << x << ", " << z << ") is a slime chunk." << std::endl;
    } else {
        std::cout << "(" << x << ", " << z << ") is not a slime chunk." << std::endl;
    }
    return 0;
}