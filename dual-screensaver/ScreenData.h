#pragma once
#include <vector>
#include <string>
#include <windows.h>

struct MazeCell {
    bool visited;
    bool wallTop, wallRight, wallBottom, wallLeft;
    bool solveVisited;
    bool inPath;
};

struct FlowParticle {
    float x, y;
    float prev_x, prev_y;
    int life;
};

struct ScreenData {
    bool isPrimary;
    bool isPreview;
    float A = 0;
    float B = 0;
    int cols = 0;
    int rows = 0;
    int stride = 0;
    std::vector<unsigned char> grid;
    std::vector<unsigned char> nextGrid;
    std::vector<uint32_t> pixels;
    DWORD lastGolUpdate = 0;
    HFONT hFont = NULL;

    std::vector<int> matrixDrops;
    std::vector<wchar_t> matrixChars;
    std::vector<unsigned char> matrixIntensity;
    HFONT hMatrixFont = NULL;

    DWORD startTime = 0;

    struct Star { float x, y, z; };
    std::vector<Star> stars;

    float logoX = 0, logoY = 0, logoDX = 3.0f, logoDY = 2.5f;
    int logoColorIndex = 0;

    std::vector<std::string> hexGrid;
    int hexCols = 0;
    int hexRows = 0;
    int activeRow = 0;
    int activeCol = 0;
    bool isRowActive = true;
    DWORD lastHexUpdate = 0;

    float pBallX = -1.0f;
    float pBallY = -1.0f;
    float pBallDX = 0.0f;
    float pBallDY = 0.0f;
    float pPadLeftY = 0.0f;
    float pPadRightY = 0.0f;

    std::vector<MazeCell> mazeGrid;
    int mazeCols = 0;
    int mazeRows = 0;
    int mazeState = 0;
    int mazeStart = 0;
    int mazeEnd = 0;
    std::vector<int> mazeStack;
    std::vector<int> solveStack;
    DWORD lastMazeUpdate = 0;
    int mazeWaitTimer = 0;

    float digitOffset[8] = { 0 };
    char currentStr[16] = { 0 };
    char targetStr[16] = { 0 };

    std::vector<FlowParticle> flowParticles;
    float flowZOff = 0.0f;
    int perm[512];

    std::vector<int> fireGrid;
    int fireWidth = 0;
    int fireHeight = 0;
};
