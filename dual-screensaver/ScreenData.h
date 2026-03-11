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

    uint64_t hexBaseAddress = 0x00007FF000000000;
    int hexDumpScrollDelay = 0;
    HFONT hHexFont = NULL;
    int hexLastWidth = 0;

    std::vector<int> bogoArray;
    bool bogoSorted = false;
    int bogoAttempts = 0;
    long long bogoComparisons = 0;
    int bogoWaitTimer = 0;

    std::vector<int> sortArray;
    int sortState = 0;
    int sortAlgo = 0;
    int sortI = 0, sortJ = 0, sortMin = 0;
    bool sortFlag = false;
    int sortSweepIdx = 0;
    int sortWait = 0;
    int sortRed1 = -1, sortRed2 = -1;
    char sortAlgoName[32] = { 0 };
    int sortSubState = 0;
    std::vector<int> sortStack;
    std::vector<int> sortOutput;
    int sortGap = 0;
    int sortCurrSize = 1;
    int sortLeftStart = 0;
    int sortExp = 1;
    long long sortComparisons = 0;
    long long sortSwaps = 0;
};
