#pragma once
#include <vector>
#include <string>
#include <windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Texture2D;
struct IDXGISurface1;

struct HarmoPendulum {
    double amp;    
    double freq;   
    double phase; 
    double damp;  
};

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

struct AntState {
    int x, y;
    int dir;
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

    std::vector<AntState> ants;
    std::vector<unsigned char> antGrid;
    std::vector<unsigned char> cyclicNext;  
    int antCols = 0;
    int antRows = 0;
    unsigned int currentAntColor;

    std::vector<float> boidVX, boidVY;

    std::vector<int>      pipeHeads;
    std::vector<int>      pipeDirs;
    std::vector<COLORREF> pipeColors;

    // Mandelbrot zoom
    double mandCX        = -0.74364990000; // zoom centre — real part
    double mandCY        =  0.13182590000; // zoom centre — imaginary part
    double mandScale     = 3.5;            // current view width in complex units
    double mandPalOff    = 0.0;            // palette phase (slowly rotates colours)
    int    mandTargetIdx = 0;              // index into curated target list
    DWORD  mandLastTick  = 0;             // for frame-rate-independent zoom

    // Clifford Attractor
    double cliffordT = 0.0;
    double cliffordX = 0.1;
    double cliffordY = 0.1;
    DWORD  cliffordLastTick = 0;

    // Triple-Pendulum Harmonograph
    HarmoPendulum harmoP[3];           // the three pendulums
    double harmoT       = 0.0;         // current curve parameter
    double harmoGlobalT = 0.0;         // global time for respawn logic
    double harmoColorT  = 0.0;         // slow hue-cycle accumulator
    DWORD  harmoLastTick = 0;
    bool   harmoInitialized = false;

    // Bad Apple ASCII Player
    bool badAppleLoaded = false;
    bool badAppleLoadAttempted = false;
    DWORD badAppleStartTime = 0;
    int badAppleTotalFrames = 0;
    int badAppleWidth = 0;
    int badAppleHeight = 0;
    int badAppleFPS = 30;
    int badAppleLevels = 16;
    uint32_t badAppleDataOffset = 0;
    std::vector<uint32_t> badAppleOffsets;
    std::vector<uint8_t> badAppleRleData;
    std::vector<uint8_t> badAppleFrameBuffer;
    HFONT badAppleFont = nullptr;
    int badAppleFontHeight = 0;

    // ASCIIQuarium
    struct AquaFish {
        float x, y;
        float vx;
        int type;
        COLORREF color;
        int animFrame;
    };
    struct AquaBubble {
        float x, y;
        float speed;
        float swaySpeed;
        float swayPhase;
        int type;
    };
    struct AquaSeaweed {
        int x;
        int height;
        float phase;
        COLORREF color;
    };
    struct AquaJellyfish {
        float x, y;
        float baseX;
        float vy;
        float pulsePhase;
        COLORREF color;
        int state;        // 0 = swimming up, 1 = descending
        float topLimit;   // target row near surface before turning back down
        float swayOffset;
    };
    struct AquaCrab {
        float x;
        int y;
        float vx;
        int animFrame;
    };
    struct AquaSurfaceEntity {
        float x;
        float vx;
        bool active;
        int type; // 0 = Duck, 1 = Sailing Ship, 2 = Whale
        float animTimer;
    };

    bool aquaInitialized = false;
    DWORD aquaLastTick = 0;
    int aquaWidthInChars = 0;
    int aquaHeightInChars = 0;
    std::vector<AquaFish> aquaFish;
    std::vector<AquaBubble> aquaBubbles;
    std::vector<AquaSeaweed> aquaSeaweed;
    std::vector<AquaJellyfish> aquaJelly;
    AquaCrab aquaCrab = { 10.0f, 0, 0.4f, 0 };
    AquaSurfaceEntity aquaSurface = { -15.0f, 0.25f, true, 0, 0.0f };

    // cbonsai (Procedural Bonsai Trees)
    struct BonsaiCell {
        char ch = ' ';
        COLORREF color = 0;
    };
    struct BonsaiShoot {
        float x, y;
        float dx, dy;
        int age;
        int maxAge;
        int generation;
        int thickness;
    };
    struct BonsaiPetal {
        float x, y;
        float vx, vy;
        float phase;
        char ch;
        COLORREF color;
    };

    bool bonsaiInitialized = false;
    DWORD bonsaiLastTick = 0;
    int bonsaiWidthInChars = 0;
    int bonsaiHeightInChars = 0;
    int bonsaiState = 0; // 0: growing, 1: mature/petals, 2: renew
    DWORD bonsaiStateStartTime = 0;
    int bonsaiTheme = 0; // 0: Spring, 1: Sakura, 2: Autumn, 3: Ginkgo
    std::vector<BonsaiCell> bonsaiGrid;
    std::vector<BonsaiShoot> bonsaiActiveShoots;
    std::vector<BonsaiPetal> bonsaiPetals;
    std::vector<POINT> bonsaiLeaves;

    // Nyan Cat (ASCII)
    struct NyanStar {
        float x, y;
        float speed;
        int type;
        int phase;
    };
    bool nyanInitialized = false;
    DWORD nyanLastTick = 0;
    DWORD nyanStartTime = 0;
    int nyanFrameIndex = 0;
    HFONT nyanFont = NULL;
    int nyanFontSize = 0;
    int nyanWidthInChars = 0;
    int nyanHeightInChars = 0;
    std::vector<NyanStar> nyanStars;

    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11Texture2D* pBackBuffer = nullptr;
    IDXGISurface1* pSurface = nullptr;
};