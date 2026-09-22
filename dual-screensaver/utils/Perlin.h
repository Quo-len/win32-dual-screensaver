#pragma once

void initPerlin(unsigned int seed, int* permArray);
float fade(float t);
float lerp(float t, float a, float b);
float grad(int hash, float x, float y, float z);
float perlin(float x, float y, float z, const int* perm);
