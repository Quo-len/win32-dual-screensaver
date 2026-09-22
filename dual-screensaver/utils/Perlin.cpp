#include "framework.h"
#include "Perlin.h"
#include <math.h>
#include <random>
#include <numeric>

void initPerlin(unsigned int seed, int* permArray) {
    int p[256];
    std::iota(p, p + 256, 0);

    std::mt19937 gen(seed);
    for (int i = 255; i > 0; i--) {
        std::uniform_int_distribution<int> dist(0, i);
        int swapIndex = dist(gen);
        int temp = p[i];
        p[i] = p[swapIndex];
        p[swapIndex] = temp;
    }

    for (int i = 0; i < 512; i++) {
        permArray[i] = p[i & 255];
    }
}

float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float lerp(float t, float a, float b) { return a + t * (b - a); }
float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y, v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}
float perlin(float x, float y, float z, const int* perm) {
    int X = (int)floor(x) & 255, Y = (int)floor(y) & 255, Z = (int)floor(z) & 255;
    x -= floor(x); y -= floor(y); z -= floor(z);
    float u = fade(x), v = fade(y), w = fade(z);
    int A = perm[X] + Y, AA = perm[A] + Z, AB = perm[A + 1] + Z;
    int B = perm[X + 1] + Y, BA = perm[B] + Z, BB = perm[B + 1] + Z;
    return lerp(w, lerp(v, lerp(u, grad(perm[AA], x, y, z),
        grad(perm[BA], x - 1, y, z)),
        lerp(u, grad(perm[AB], x, y - 1, z),
            grad(perm[BB], x - 1, y - 1, z))),
        lerp(v, lerp(u, grad(perm[AA + 1], x, y, z - 1),
            grad(perm[BA + 1], x - 1, y, z - 1)),
            lerp(u, grad(perm[AB + 1], x, y - 1, z - 1),
                grad(perm[BB + 1], x - 1, y - 1, z - 1))));
}
