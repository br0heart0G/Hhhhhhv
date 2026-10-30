// fps_unlocker.cpp
// ─────────────────────────────────────────────────────────────────────────────
//  GTA San Andreas — Self-Loading DLL (d3d9.dll proxy)
//  Features:
//    1. 200 FPS unlock
//    2. Thread pool expansion: 96 → 200
//    3. main2.scm auto-loader from data/script/
//
//  Build MSVC:
//    cl /LD /O2 fps_unlocker.cpp /Fe:d3d9.dll d3d9.lib user32.lib winmm.lib
//       /link /DLL /SUBSYSTEM:WINDOWS
//
//  Build MinGW:
//    g++ -shared -O2 -o d3d9.dll fps_unlocker.cpp
//        -ld3d9 -lwinmm -luser32 -mwindows -static-libgcc -static-libstdc++
//
//  نصب:
//    فقط d3d9.dll رو کنار gta_sa.exe بذار — هیچ loader ای لازم نیست.
// ─────────────────────────────────────────────────────────────────────────────

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

// ═════════════════════════════════════════════════════════════════════════════
//  PART 0 — d3d9.dll PROXY
//  همه export های real d3d9 رو forward میکنیم
// ═════════════════════════════════════════════════════════════════════════════

static HMODULE g_realD3D9 = nullptr;

// تمام function pointer های d3d9
typedef IDirect3D9* (WINAPI* PFN_D3DCreate9)(UINT);
typedef HRESULT     (WINAPI* PFN_D3DCreate9Ex)(UINT, IDirect3D9Ex**);
typedef void*       (WINAPI* PFN_D3DShaderValidatorCreate9)();
typedef int         (WINAPI* PFN_D3DPERF_BeginEvent)(D3DCOLOR, LPCWSTR);
typedef int         (WINAPI* PFN_D3DPERF_EndEvent)();
typedef DWORD       (WINAPI* PFN_D3DPERF_GetStatus)();
typedef BOOL        (WINAPI* PFN_D3DPERF_QueryRepeatFrame)();
typedef void        (WINAPI* PFN_D3DPERF_SetMarker)(D3DCOLOR, LPCWSTR);
typedef void        (WINAPI* PFN_D3DPERF_SetOptions)(DWORD);
typedef void        (WINAPI* PFN_D3DPERF_SetRegion)(D3DCOLOR, LPCWSTR);
typedef HRESULT     (WINAPI* PFN_DebugSetLevel)(DWORD);
typedef void        (WINAPI* PFN_DebugSetMute)();
typedef void*       (WINAPI* PFN_PSGPError)();
typedef void*       (WINAPI* PFN_PSGPSampleTexture)();

static PFN_D3DCreate9                g_pfnCreate9                = nullptr;
static PFN_D3DCreate9Ex              g_pfnCreate9Ex              = nullptr;
static PFN_D3DShaderValidatorCreate9 g_pfnShaderValidator        = nullptr;
static PFN_D3DPERF_BeginEvent        g_pfnPerfBegin              = nullptr;
static PFN_D3DPERF_EndEvent          g_pfnPerfEnd                = nullptr;
static PFN_D3DPERF_GetStatus         g_pfnPerfGetStatus          = nullptr;
static PFN_D3DPERF_QueryRepeatFrame  g_pfnPerfQueryRepeat        = nullptr;
static PFN_D3DPERF_SetMarker         g_pfnPerfSetMarker          = nullptr;
static PFN_D3DPERF_SetOptions        g_pfnPerfSetOptions         = nullptr;
static PFN_D3DPERF_SetRegion         g_pfnPerfSetRegion          = nullptr;
static PFN_DebugSetLevel             g_pfnDebugSetLevel          = nullptr;
static PFN_DebugSetMute              g_pfnDebugSetMute           = nullptr;

static bool LoadRealD3D9()
{
    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    strcat_s(sysPath, "\\d3d9.dll");
    g_realD3D9 = LoadLibraryA(sysPath);
    if (!g_realD3D9) return false;

#define GETPROC(fn, pfn) pfn = (decltype(pfn))GetProcAddress(g_realD3D9, fn)
    GETPROC("Direct3DCreate9",                  g_pfnCreate9);
    GETPROC("Direct3DCreate9Ex",                g_pfnCreate9Ex);
    GETPROC("Direct3DShaderValidatorCreate9",   g_pfnShaderValidator);
    GETPROC("D3DPERF_BeginEvent",               g_pfnPerfBegin);
    GETPROC("D3DPERF_EndEvent",                 g_pfnPerfEnd);
    GETPROC("D3DPERF_GetStatus",                g_pfnPerfGetStatus);
    GETPROC("D3DPERF_QueryRepeatFrame",         g_pfnPerfQueryRepeat);
    GETPROC("D3DPERF_SetMarker",                g_pfnPerfSetMarker);
    GETPROC("D3DPERF_SetOptions",               g_pfnPerfSetOptions);
    GETPROC("D3DPERF_SetRegion",                g_pfnPerfSetRegion);
    GETPROC("DebugSetLevel",                    g_pfnDebugSetLevel);
    GETPROC("DebugSetMute",                     g_pfnDebugSetMute);
#undef GETPROC
    return true;
}

// ─── Proxy Exports ───────────────────────────────────────────────────────────
extern "C" {

__declspec(dllexport) IDirect3D9* WINAPI
Direct3DCreate9(UINT sdkVer)
{
    return g_pfnCreate9 ? g_pfnCreate9(sdkVer) : nullptr;
}

__declspec(dllexport) HRESULT WINAPI
Direct3DCreate9Ex(UINT sdkVer, IDirect3D9Ex** ppD3D)
{
    return g_pfnCreate9Ex ? g_pfnCreate9Ex(sdkVer, ppD3D) : E_FAIL;
}

__declspec(dllexport) void* WINAPI
Direct3DShaderValidatorCreate9()
{
    return g_pfnShaderValidator ? g_pfnShaderValidator() : nullptr;
}

__declspec(dllexport) int WINAPI
D3DPERF_BeginEvent(D3DCOLOR c, LPCWSTR n)
{ return g_pfnPerfBegin ? g_pfnPerfBegin(c, n) : 0; }

__declspec(dllexport) int WINAPI
D3DPERF_EndEvent()
{ return g_pfnPerfEnd ? g_pfnPerfEnd() : 0; }

__declspec(dllexport) DWORD WINAPI
D3DPERF_GetStatus()
{ return g_pfnPerfGetStatus ? g_pfnPerfGetStatus() : 0; }

__declspec(dllexport) BOOL WINAPI
D3DPERF_QueryRepeatFrame()
{ return g_pfnPerfQueryRepeat ? g_pfnPerfQueryRepeat() : FALSE; }

__declspec(dllexport) void WINAPI
D3DPERF_SetMarker(D3DCOLOR c, LPCWSTR n)
{ if (g_pfnPerfSetMarker) g_pfnPerfSetMarker(c, n); }

__declspec(dllexport) void WINAPI
D3DPERF_SetOptions(DWORD opt)
{ if (g_pfnPerfSetOptions) g_pfnPerfSetOptions(opt); }

__declspec(dllexport) void WINAPI
D3DPERF_SetRegion(D3DCOLOR c, LPCWSTR n)
{ if (g_pfnPerfSetRegion) g_pfnPerfSetRegion(c, n); }

__declspec(dllexport) HRESULT WINAPI
DebugSetLevel(DWORD lvl)
{ return g_pfnDebugSetLevel ? g_pfnDebugSetLevel(lvl) : S_OK; }

__declspec(dllexport) void WINAPI
DebugSetMute()
{ if (g_pfnDebugSetMute) g_pfnDebugSetMute(); }

} // extern "C"


// ═════════════════════════════════════════════════════════════════════════════
//  PART 1 — MEMORY PATCH HELPERS
// ═════════════════════════════════════════════════════════════════════════════

static void PatchBytes(uintptr_t addr, const uint8_t* data, size_t len)
{
    DWORD old;
    void* ptr = (void*)addr;
    VirtualProtect(ptr, len, PAGE_EXECUTE_READWRITE, &old);
    memcpy(ptr, data, len);
    VirtualProtect(ptr, len, old, &old);
    FlushInstructionCache(GetCurrentProcess(), ptr, len);
}

template<typename T>
static void PatchVal(uintptr_t addr, T val)
{
    PatchBytes(addr, (const uint8_t*)&val, sizeof(T));
}

// ═════════════════════════════════════════════════════════════════════════════
//  PART 2 — FPS UNLOCK (200 FPS)
// ═════════════════════════════════════════════════════════════════════════════

static constexpr int   TARGET_FPS      = 200;
static constexpr float TARGET_FRAME_MS = 1000.0f / TARGET_FPS;

// SA 1.0 US addresses
static constexpr uintptr_t ADDR_FPS_BOOL     = 0x00C1703C;
static constexpr uintptr_t ADDR_FPS_DELAY    = 0x00C17040;
static constexpr uintptr_t ADDR_FPS_CALL     = 0x0053E943;

static void PatchFPS()
{
    PatchVal<uint8_t>(ADDR_FPS_BOOL,  0x00);
    PatchVal<float>  (ADDR_FPS_DELAY, TARGET_FRAME_MS);
    static const uint8_t nop5[] = {0x90,0x90,0x90,0x90,0x90};
    PatchBytes(ADDR_FPS_CALL, nop5, 5);
}

// D3D9 Present hook برای per-frame cap
typedef HRESULT(APIENTRY* PFN_Present)(
    IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
static PFN_Present g_oPresent = nullptr;

static LARGE_INTEGER g_freq     = {};
static LARGE_INTEGER g_lastTime = {};
static bool          g_timerOK  = false;

static void CapFPS()
{
    if (!g_timerOK) return;
    LONGLONG tpf = g_freq.QuadPart / TARGET_FPS;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    LONGLONG elapsed = now.QuadPart - g_lastTime.QuadPart;
    if (elapsed < tpf)
    {
        LONGLONG rem = tpf - elapsed;
        LONGLONG sleepT = rem - g_freq.QuadPart / 2000;
        if (sleepT > 0)
        {
            DWORD ms = (DWORD)((sleepT * 1000) / g_freq.QuadPart);
            if (ms > 0) Sleep(ms - 1);
        }
        do { QueryPerformanceCounter(&now); }
        while ((now.QuadPart - g_lastTime.QuadPart) < tpf);
    }
    g_lastTime = now;
}

static HRESULT APIENTRY hkPresent(
    IDirect3DDevice9* dev, const RECT* s, const RECT* d,
    HWND hw, const RGNDATA* rg)
{
    CapFPS();
    return g_oPresent(dev, s, d, hw, rg);
}

static void HookPresent()
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return;

    D3DPRESENT_PARAMETERS pp = {};
    pp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    pp.Windowed         = TRUE;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.BackBufferCount  = 1;
    pp.BackBufferWidth  = 2;
    pp.BackBufferHeight = 2;
    pp.hDeviceWindow    = GetDesktopWindow();

    IDirect3DDevice9* dev = nullptr;
    if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF,
        pp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev)))
    {
        d3d->Release();
        return;
    }

    void** vmt = *(void***)dev;
    DWORD old;
    VirtualProtect(&vmt[17], sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    g_oPresent = (PFN_Present)vmt[17];
    vmt[17]    = (void*)hkPresent;
    VirtualProtect(&vmt[17], sizeof(void*), old, &old);

    dev->Release();
    d3d->Release();
}


// ═════════════════════════════════════════════════════════════════════════════
//  PART 3 — THREAD POOL EXPANSION: 96 → 200
//
//  SA 1.0 US:
//    CTheScripts::ScriptsArray  = 0x00A8D040   (96 × 0xE4 bytes)
//    CRunningScript size        = 0xE4  (228 bytes)
//    Thread count refs (96/0x60) پچ میشن
// ═════════════════════════════════════════════════════════════════════════════

static constexpr int    OLD_THREAD_COUNT    = 96;
static constexpr int    NEW_THREAD_COUNT    = 200;
static constexpr size_t SCRIPT_STRUCT_SIZE  = 0xE4;  // sizeof(CRunningScript) SA 1.0

static constexpr uintptr_t ADDR_SCRIPTS_ARRAY = 0x00A8D040;

// آدرس هایی که عدد 96 (0x60) رو دارن و باید پچ بشن — SA 1.0 US
// این ها loop bound های CTheScripts::Process و StartNewScript هستن
struct CountPatch {
    uintptr_t addr;
    uint8_t   bytes[4];
    size_t    len;
};

// جدول پچ های thread count
static const CountPatch THREAD_COUNT_PATCHES[] = {
    // CTheScripts::Process — loop counter: CMP reg, 96
    { 0x0045D690, {0xC8, 0x00, 0x00, 0x00}, 4 }, // 200 = 0xC8
    // StartNewScript — upper bound check: CMP [idx], 96  
    { 0x0046A4E0, {0xC8, 0x00, 0x00, 0x00}, 4 },
    // UpdateCompareFlag loop — 96
    { 0x004800A5, {0xC8, 0x00, 0x00, 0x00}, 4 },
};

// Buffer جدید برای thread pool (heap allocated)
static uint8_t* g_newScriptPool = nullptr;

static void ExpandThreadPool()
{
    // 1. Allocate new pool: 200 × 228 bytes
    size_t newSize = NEW_THREAD_COUNT * SCRIPT_STRUCT_SIZE;
    g_newScriptPool = (uint8_t*)VirtualAlloc(
        nullptr, newSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    if (!g_newScriptPool) return;

    // 2. کپی 96 thread قدیمی به pool جدید
    memcpy(g_newScriptPool,
           (void*)ADDR_SCRIPTS_ARRAY,
           OLD_THREAD_COUNT * SCRIPT_STRUCT_SIZE);

    // 3. بقیه thread ها رو zero کن
    memset(g_newScriptPool + OLD_THREAD_COUNT * SCRIPT_STRUCT_SIZE,
           0,
           (NEW_THREAD_COUNT - OLD_THREAD_COUNT) * SCRIPT_STRUCT_SIZE);

    // 4. آدرس ScriptsArray رو به pool جدید patch کن
    //    هر جایی که به 0x00A8D040 اشاره داره رو redirect میکنیم
    //    SA 1.0 US — آدرس های مستقیم ScriptsArray در کد:
    static const uintptr_t ARRAY_REF_ADDRS[] = {
        0x0045D650,  // CTheScripts::Process — lea/mov base
        0x0046A4B5,  // StartNewScript — array base
        0x00480090,  // UpdateCompareFlag — array base
        0x004800D0,  // ClearMemory — array base
    };

    for (auto refAddr : ARRAY_REF_ADDRS)
    {
        // این addr ها معمولاً MOV [reg], imm32 یا LEA رو DWORD ptr دارن
        PatchVal<uint32_t>(refAddr + 3, (uint32_t)(uintptr_t)g_newScriptPool);
    }

    // 5. Thread count رو از 96 به 200 patch کن
    for (auto& p : THREAD_COUNT_PATCHES)
    {
        PatchBytes(p.addr, p.bytes, p.len);
    }
}


// ═════════════════════════════════════════════════════════════════════════════
//  PART 4 — main2.scm LOADER
//
//  - فایل data/script/main2.scm رو میخونه
//  - یه CRunningScript جدید میسازه که IP اش اول main2.scm باشه
//  - Thread های داخل main2.scm از طریق opcode 004F خودشون fork میکنن
//  - حداکثر thread ها از pool جدید (200 تا) استفاده میکنن
// ═════════════════════════════════════════════════════════════════════════════

// SA 1.0 US — CTheScripts::StartNewScript
static constexpr uintptr_t ADDR_START_NEW_SCRIPT = 0x0046A4B0;

// CRunningScript ساختار ساده (فقط فیلدهایی که نیاز داریم)
#pragma pack(push, 1)
struct CRunningScript
{
    uint8_t  _pad0[8];        // 0x00: next/prev pointers
    char     m_szName[8];     // 0x08: thread name
    uint8_t* m_pBaseIP;       // 0x10: base IP (اول script)
    uint8_t* m_pIP;           // 0x14: current IP
    uint8_t* m_pStack[8];     // 0x18: call stack
    uint16_t m_nStackPointer; // 0x38
    uint8_t  _pad1[2];
    int32_t  m_anLocalVariables[40]; // 0x3C: local vars (0x3C to 0xDB)
    uint8_t  m_bIsActive;     // 0xDC
    uint8_t  m_bCondResult;   // 0xDD
    uint8_t  _pad2[6];        // 0xDE..0xE3
    // total: 0xE4 = 228 bytes
};
#pragma pack(pop)

// تابع StartNewScript: __cdecl (SA 1.0 US)
// CRunningScript* StartNewScript(uint8_t* startIP)
typedef CRunningScript* (__cdecl* PFN_StartNewScript)(uint8_t* startIP);

static uint8_t* g_scm2Buffer   = nullptr;
static DWORD    g_scm2Size     = 0;

static void LoadMain2SCM()
{
    // مسیر نسبی به گیم
    char gamePath[MAX_PATH];
    GetModuleFileNameA(nullptr, gamePath, MAX_PATH);
    // آخرین \ رو پیدا کن و اسم exe رو حذف کن
    char* lastSlash = strrchr(gamePath, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';

    char scmPath[MAX_PATH];
    sprintf_s(scmPath, "%sdata\\script\\main2.scm", gamePath);

    HANDLE hFile = CreateFileA(
        scmPath, GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        // فایل نیست — مشکلی نیست، skip
        return;
    }

    g_scm2Size = GetFileSize(hFile, nullptr);
    if (g_scm2Size == 0 || g_scm2Size == INVALID_FILE_SIZE)
    {
        CloseHandle(hFile);
        return;
    }

    // Buffer executable بگیر چون SA IP رو مستقیم execute میکنه
    g_scm2Buffer = (uint8_t*)VirtualAlloc(
        nullptr, g_scm2Size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);

    if (!g_scm2Buffer)
    {
        CloseHandle(hFile);
        return;
    }

    DWORD bytesRead = 0;
    ReadFile(hFile, g_scm2Buffer, g_scm2Size, &bytesRead, nullptr);
    CloseHandle(hFile);

    if (bytesRead != g_scm2Size)
    {
        VirtualFree(g_scm2Buffer, 0, MEM_RELEASE);
        g_scm2Buffer = nullptr;
        return;
    }

    // StartNewScript رو call کن با شروع buffer
    // این یه thread جدید میسازه که از byte 0 اجرا میشه
    auto pfnStart = (PFN_StartNewScript)ADDR_START_NEW_SCRIPT;
    CRunningScript* newThread = pfnStart(g_scm2Buffer);

    if (newThread)
    {
        // اسم thread رو بذار
        strncpy_s(newThread->m_szName, "MAIN2", 8);
        newThread->m_bIsActive   = 1;
        newThread->m_pBaseIP     = g_scm2Buffer;
        newThread->m_bCondResult = 0;
    }
}


// ═════════════════════════════════════════════════════════════════════════════
//  PART 5 — INIT THREAD
// ═════════════════════════════════════════════════════════════════════════════

// Hook روی CTheScripts::Init برای اینکه بعد از اینکه SA خودش
// scripts رو load کرد، ما main2.scm رو بذاریم
typedef void (__cdecl* PFN_ScriptsInit)();
static PFN_ScriptsInit g_oScriptsInit = nullptr;

static void __cdecl hkScriptsInit()
{
    // اول اجازه بده SA خودش init کنه
    if (g_oScriptsInit) g_oScriptsInit();

    // بعد ما main2 رو load میکنیم
    LoadMain2SCM();
}

// SA 1.0 US: CTheScripts::Init at 0x0045D440
static constexpr uintptr_t ADDR_SCRIPTS_INIT = 0x0045D440;

static void HookScriptsInit()
{
    // 5-byte CALL/JMP relative detour
    uint8_t* target = (uint8_t*)ADDR_SCRIPTS_INIT;
    DWORD old;
    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);

    g_oScriptsInit = (PFN_ScriptsInit)(uintptr_t)(
        ADDR_SCRIPTS_INIT + 5 +
        *(int32_t*)(target + 1));

    // overwrite با JMP به هوک ما
    target[0] = 0xE9;
    *(int32_t*)(target + 1) =
        (int32_t)((uintptr_t)hkScriptsInit - ADDR_SCRIPTS_INIT - 5);

    VirtualProtect(target, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
}


// ═════════════════════════════════════════════════════════════════════════════
//  PART 6 — MAIN INIT THREAD
// ═════════════════════════════════════════════════════════════════════════════

static DWORD WINAPI InitThread(LPVOID)
{
    // صبر کن SA کامل load بشه
    Sleep(3000);

    // Timer برای FPS cap
    if (QueryPerformanceFrequency(&g_freq))
    {
        QueryPerformanceCounter(&g_lastTime);
        g_timerOK = true;
    }

    // Windows timer resolution → 1ms
    timeBeginPeriod(1);

    // ── FPS Unlock ──────────────────────────────
    PatchFPS();
    HookPresent();

    // ── Thread Pool Expansion ────────────────────
    // این باید قبل از اینکه SA اولین script رو load کنه باشه
    // ولی چون ما داریم ScriptsInit رو hook میکنیم، ایمنه
    ExpandThreadPool();

    // ── main2.scm Hook ───────────────────────────
    HookScriptsInit();

    return 0;
}


// ═════════════════════════════════════════════════════════════════════════════
//  DllMain
// ═════════════════════════════════════════════════════════════════════════════

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hMod);

        // اول real d3d9 رو load کن
        if (!LoadRealD3D9()) return FALSE;

        CloseHandle(CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr));
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        timeEndPeriod(1);

        if (g_newScriptPool)
            VirtualFree(g_newScriptPool, 0, MEM_RELEASE);

        if (g_scm2Buffer)
            VirtualFree(g_scm2Buffer, 0, MEM_RELEASE);

        if (g_realD3D9)
            FreeLibrary(g_realD3D9);
    }
    return TRUE;
}