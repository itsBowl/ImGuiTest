#pragma once
#include <string>
//perlin noise from https://www.shadertoy.com/view/NlSGDz
std::string perlinShaderCode = R"SHADER(
uint hash (uint x, uint seed)
{
    const uint m = 0x5BD1E995;
    uint hash = seed;
    uint k = x;
    k *= m;
    k ^= k >> 24;
    k *= m;
    hash *= m;
    hash ^= k;
    hash *= m;
    hash ^= hash >> 15;
    return hash;
}

uint hash(uvec2 x, uint seed)
{
    const uint m = 0x5BD1E995;
    uint hash = seed;
    uint k = x.x;
    k *= m;
    k ^= k >> 24;
    k *= m;
    hash *= m;
    hash ^= k;
    hash *= m;
    hash ^= hash >> 15;
    k = x.y;
    k *= m;
    k ^= k >> 24;
    k *= m;
    hash *= m;
    hash ^= k;
    hash *= m;
    hash ^= hash >> 15;

    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;

    return hash;
}

vec2 gradientDirection(uint hash)
{
    switch (int(hash) & 3) { 
    case 0:
        return vec2(1.0, 1.0);
    case 1:
        return vec2(-1.0, 1.0);
    case 2:
        return vec2(1.0, -1.0);
    case 3:
        return vec2(-1.0, -1.0);
    }
}

float interpolate(float value1, float value2, float value3, float value4, vec2 t) 
{
    return mix(mix(value1, value2, t.x), mix(value3, value4, t.x), t.y);
}

vec2 fade(vec2 t) 
{
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float perlinNoise(vec2 position, uint seed) 
{
    vec2 floorPosition = floor(position);
    vec2 fractPosition = position - floorPosition;
    uvec2 cellCoordinates = uvec2(floorPosition);
    float value1 = dot(gradientDirection(hash(cellCoordinates, seed)), fractPosition);
    float value2 = dot(gradientDirection(hash((cellCoordinates + uvec2(1, 0)), seed)), fractPosition - vec2(1.0, 0.0));
    float value3 = dot(gradientDirection(hash((cellCoordinates + uvec2(0, 1)), seed)), fractPosition - vec2(0.0, 1.0));
    float value4 = dot(gradientDirection(hash((cellCoordinates + uvec2(1, 1)), seed)), fractPosition - vec2(1.0, 1.0));
    return interpolate(value1, value2, value3, value4, fade(fractPosition));
}

float perlinNoise(vec2 position, float frequency, float octaves, float persistence, float lacunarity, float s) 
{
    uint seed = uint(s);
    int octaveCount = int(octaves);
    float value = 0.0;
    float amplitude = 1.0;
    float currentFrequency = float(frequency);
    uint currentSeed = seed;
    for (int i = 0; i < octaveCount; i++) {
        currentSeed = hash(currentSeed, 0x0U);
        value += perlinNoise(position * currentFrequency, currentSeed) * amplitude;
        amplitude *= persistence;
        currentFrequency *= lacunarity;
    }
    return value;
}

float simpleNoise(vec2 position, float frequency, float octaves, float s)
{
    return perlinNoise(position, frequency, octaves, 1.0f, 0.5f, s);
}
)SHADER";

//104 lines in extra shader code

std::string targetShader = R"TGT(
#version 450 core
in vec2 vUV;
in float time;
layout (location = 0) out vec4 fragColour;
uint hash (uint x, uint seed)
{
    const uint m = 0x5BD1E995;
    uint hash = seed;
    uint k = x;
    k *= m;
    k ^= k >> 24;
    k *= m;
    hash *= m;
    hash ^= k;
    hash *= m;
    hash ^= hash >> 15;
    return hash;
}

uint hash(uvec2 x, uint seed)
{
    const uint m = 0x5BD1E995;
    uint hash = seed;
    uint k = x.x;
    k *= m;
    k ^= k >> 24;
    k *= m;
    hash *= m;
    hash ^= k;
    hash *= m;
    hash ^= hash >> 15;
    k = x.y;
    k *= m;
    k ^= k >> 24;
    k *= m;
    hash *= m;
    hash ^= k;
    hash *= m;
    hash ^= hash >> 15;

    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;

    return hash;
}

vec2 gradientDirection(uint hash)
{
    switch (int(hash) & 3) { 
    case 0:
        return vec2(1.0, 1.0);
    case 1:
        return vec2(-1.0, 1.0);
    case 2:
        return vec2(1.0, -1.0);
    case 3:
        return vec2(-1.0, -1.0);
    }
}

float interpolate(float value1, float value2, float value3, float value4, vec2 t) 
{
    return mix(mix(value1, value2, t.x), mix(value3, value4, t.x), t.y);
}

vec2 fade(vec2 t) 
{
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float perlinNoise(vec2 position, uint seed) 
{
    vec2 floorPosition = floor(position);
    vec2 fractPosition = position - floorPosition;
    uvec2 cellCoordinates = uvec2(floorPosition);
    float value1 = dot(gradientDirection(hash(cellCoordinates, seed)), fractPosition);
    float value2 = dot(gradientDirection(hash((cellCoordinates + uvec2(1, 0)), seed)), fractPosition - vec2(1.0, 0.0));
    float value3 = dot(gradientDirection(hash((cellCoordinates + uvec2(0, 1)), seed)), fractPosition - vec2(0.0, 1.0));
    float value4 = dot(gradientDirection(hash((cellCoordinates + uvec2(1, 1)), seed)), fractPosition - vec2(1.0, 1.0));
    return interpolate(value1, value2, value3, value4, fade(fractPosition));
}

float perlinNoise(vec2 position, float frequency, float octaves, float persistence, float lacunarity, float s) 
{
    uint seed = uint(s);
    int octaveCount = int(octaves);
    float value = 0.0;
    float amplitude = 1.0;
    float currentFrequency = float(frequency);
    uint currentSeed = seed;
    for (int i = 0; i < octaveCount; i++) {
        currentSeed = hash(currentSeed, 0x0U);
        value += perlinNoise(position * currentFrequency, currentSeed) * amplitude;
        amplitude *= persistence;
        currentFrequency *= lacunarity;
    }
    return value;
}

float simpleNoise(vec2 position, float frequency, float octaves, float s)
{
    return perlinNoise(position, frequency, octaves, 1.0f, 0.5f, s);
}


void main() {
vec4 var6 = vec4(vUV.x, vUV.y, 0.0f, 0.0f);
vec4 scaleUV = var6 * 2.0f;
float var82 = simpleNoise(vec2(scaleUV), 5.0f, 2.0f, 0.0f); 
float var102 = pow(2.0f, 5.0f);
float NoiseSeed = 2.0f + var102;
float var88 = simpleNoise(vec2(scaleUV), 5.0f, 2.0f, NoiseSeed); 
vec4 combineNoise = vec4(var82, var88, 0.0f, 0.0f);
vec4 var114 = combineNoise + 1.0f;
vec4 var122 = var114 * 0.5f;
vec4 var16 = mod(scaleUV, 1.0f);
vec4 var22 = var16 - 0.5f;
float var49 = sin(time );
float getCyclicTime = var49 * 10.0f;
float addCycles = getCyclicTime * var22.y;
float limit = min(addCycles, 1.0f);
vec4 var64 = vec4(var22.x, limit, 0.0f, 0.0f);
float var32 = length(var64);
float var28 = 1.0f - var32;
float var37 = pow(var28, 3.0f);
float var41 = var37 * 10.0f;
float limitValue = clamp(var41, -0.50f, 2.0f);
vec4 addAnimation = var122 * limitValue;
fragColour  = vec4(addAnimation);
}
)TGT";
