// Minimal offline VST2 host (own declarations from public behaviour; local analysis only).
// Build x86_64, run under Rosetta.
//   vsthost info
//   vsthost sweep <paramIndex> <steps>          print getParameter display strings
//   vsthost render in.f32 out.f32 [sr] [block] [--set i=v]... [--setat frame:i=v]...
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct AEffect AEffect;
typedef intptr_t (*Dispatcher)(AEffect*, int32_t, int32_t, intptr_t, void*, float);
struct AEffect {
    int32_t magic;
    Dispatcher dispatcher;
    void (*process)(AEffect*, float**, float**, int32_t);
    void (*setParameter)(AEffect*, int32_t, float);
    float (*getParameter)(AEffect*, int32_t);
    int32_t numPrograms, numParams, numInputs, numOutputs, flags;
    intptr_t resvd1, resvd2;
    int32_t initialDelay, realQualities, offQualities;
    float ioRatio;
    void* object; void* user;
    int32_t uniqueID, version;
    void (*processReplacing)(AEffect*, float**, float**, int32_t);
    void (*processDoubleReplacing)(AEffect*, double**, double**, int32_t);
    char future[56];
};

static double g_sr = 44100.0;
static intptr_t host(AEffect* e, int32_t op, int32_t idx, intptr_t val, void* ptr, float opt) {
    (void)e; (void)idx; (void)val; (void)ptr; (void)opt;
    switch (op) {
        case 1: return 2400;
        case 16: return (intptr_t)g_sr;
        case 17: return 512;
        case 23: return 2;
        case 38: return 1;
        default: return 0;
    }
}

enum { effOpen=0, effClose=1, effSetProgram=2, effGetProgram=3, effGetProgramName=5, effGetParamLabel=6,
       effGetParamDisplay=7, effGetParamName=8, effSetSampleRate=10, effSetBlockSize=11, effMainsChanged=12,
       effGetChunk=23, effSetChunk=24, effGetProgramNameIndexed=29, effGetEffectName=45, effGetVendorString=47,
       effGetProductString=48, effGetVendorVersion=49, effGetVstVersion=58, effStartProcess=71 };

static AEffect* load(const char* path) {
    CFStringRef p = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, p, kCFURLPOSIXPathStyle, true);
    CFBundleRef b = CFBundleCreate(NULL, url);
    if (!b) { fprintf(stderr, "bundle create failed\n"); exit(1); }
    if (!CFBundleLoadExecutable(b)) { fprintf(stderr, "bundle load failed\n"); exit(1); }
    typedef AEffect* (*Entry)(intptr_t (*)(AEffect*, int32_t, int32_t, intptr_t, void*, float));
    Entry entry = (Entry)CFBundleGetFunctionPointerForName(b, CFSTR("VSTPluginMain"));
    if (!entry) entry = (Entry)CFBundleGetFunctionPointerForName(b, CFSTR("main_macho"));
    if (!entry) { fprintf(stderr, "no entry point\n"); exit(1); }
    AEffect* e = entry(host);
    if (!e || e->magic != 0x56737450) { fprintf(stderr, "bad AEffect\n"); exit(1); }
    return e;
}

static void str(AEffect* e, int op, int idx, char* out) {
    memset(out, 0, 256);
    e->dispatcher(e, op, idx, 0, out, 0);
}

static void setup(AEffect* e) {
    e->dispatcher(e, effOpen, 0, 0, NULL, 0);
    e->dispatcher(e, effSetSampleRate, 0, 0, NULL, (float)g_sr);
    e->dispatcher(e, effSetBlockSize, 0, 512, NULL, 0);
    e->dispatcher(e, effMainsChanged, 0, 1, NULL, 0);
    e->dispatcher(e, effStartProcess, 0, 0, NULL, 0);
}

int main(int argc, char** argv) {
    const char* vst = getenv("CC_VST") ? getenv("CC_VST") : "vst/CamelCrusher.vst"; // relative to tools/analysis
    if (argc < 2) { fprintf(stderr, "usage\n"); return 1; }
    AEffect* e = load(vst);
    char s[256], t[256], u[256];
    if (!strcmp(argv[1], "info")) {
        setup(e);
        str(e, effGetEffectName, 0, s); str(e, effGetVendorString, 0, t); str(e, effGetProductString, 0, u);
        printf("name='%s' vendor='%s' product='%s'\n", s, t, u);
        printf("uniqueID=0x%08x version=%d flags=%d programs=%d params=%d in=%d out=%d delay=%d vst=%ld\n",
               e->uniqueID, e->version, e->flags, e->numPrograms, e->numParams, e->numInputs, e->numOutputs,
               e->initialDelay, (long)e->dispatcher(e, effGetVstVersion, 0, 0, NULL, 0));
        for (int i = 0; i < e->numParams; i++) {
            str(e, effGetParamName, i, s); str(e, effGetParamDisplay, i, t); str(e, effGetParamLabel, i, u);
            printf("param %2d  %-18s  value=%.6f  display='%s' label='%s'\n", i, s, e->getParameter(e, i), t, u);
        }
        for (int i = 0; i < e->numPrograms && i < 64; i++) {
            str(e, effGetProgramNameIndexed, i, s);
            printf("program %2d '%s'\n", i, s);
        }
        void* chunk = NULL;
        intptr_t n = e->dispatcher(e, effGetChunk, 0, 0, &chunk, 0);
        printf("bank chunk size=%ld\n", (long)n);
        return 0;
    }
    if (!strcmp(argv[1], "sweep")) {
        int idx = atoi(argv[2]); int steps = atoi(argv[3]);
        setup(e);
        for (int k = 0; k <= steps; k++) {
            float v = (float)k / steps;
            e->setParameter(e, idx, v);
            str(e, effGetParamDisplay, idx, t); str(e, effGetParamLabel, idx, u);
            printf("%.5f\t%s\t%s\n", v, t, u);
        }
        return 0;
    }
    if (!strcmp(argv[1], "render")) {
        const char* inp = argv[2]; const char* outp = argv[3];
        int block = 512; int pos = 4;
        if (argc > pos && argv[pos][0] != '-') { g_sr = atof(argv[pos++]); }
        if (argc > pos && argv[pos][0] != '-') { block = atoi(argv[pos++]); }
        setup(e);
        e->dispatcher(e, effSetSampleRate, 0, 0, NULL, (float)g_sr);
        e->dispatcher(e, effSetBlockSize, 0, block, NULL, 0);
        typedef struct { int frame, idx; float v; } Ev;
        Ev evs[4096]; int nev = 0;
        for (int i = pos; i < argc; i++) {
            if (!strcmp(argv[i], "--set") && i + 1 < argc) {
                int idx; float v; sscanf(argv[++i], "%d=%f", &idx, &v);
                e->setParameter(e, idx, v);
            } else if (!strcmp(argv[i], "--setat") && i + 1 < argc) {
                Ev ev; sscanf(argv[++i], "%d:%d=%f", &ev.frame, &ev.idx, &ev.v); evs[nev++] = ev;
            }
        }
        FILE* f = fopen(inp, "rb"); fseek(f, 0, SEEK_END); long bytes = ftell(f); fseek(f, 0, SEEK_SET);
        long frames = bytes / 8; float* inter = malloc(bytes); fread(inter, 1, bytes, f); fclose(f);
        float* L = calloc(block, 4); float* R = calloc(block, 4); float* oL = calloc(block, 4); float* oR = calloc(block, 4);
        float* out = malloc(bytes);
        long done = 0; int ei = 0;
        while (done < frames) {
            int n = (int)((frames - done) < block ? (frames - done) : block);
            while (ei < nev && evs[ei].frame <= done) { e->setParameter(e, evs[ei].idx, evs[ei].v); ei++; }
            if (ei < nev && evs[ei].frame - done < n) n = (int)(evs[ei].frame - done);
            for (int i = 0; i < n; i++) { L[i] = inter[(done + i) * 2]; R[i] = inter[(done + i) * 2 + 1]; }
            float* ins[2] = { L, R }; float* outs[2] = { oL, oR };
            e->processReplacing(e, ins, outs, n);
            for (int i = 0; i < n; i++) { out[(done + i) * 2] = oL[i]; out[(done + i) * 2 + 1] = oR[i]; }
            done += n;
        }
        f = fopen(outp, "wb"); fwrite(out, 1, bytes, f); fclose(f);
        fprintf(stderr, "ok frames=%ld\n", frames);
        return 0;
    }
    return 1;
}
