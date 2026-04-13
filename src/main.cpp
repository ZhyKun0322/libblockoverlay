#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <dlfcn.h>
#include <link.h>
#include <sys/mman.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "BlockOverlay", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "BlockOverlay", __VA_ARGS__)

// Color config - BLUE with 30% transparency
static float g_color[4] = {0.0f, 0.4f, 1.0f, 0.3f};

// Module base address
static void* g_mcBase = nullptr;
static size_t g_mcSize = 0;

// Function signatures (bytes to search for)
// These are common ARM64 function prologues near our string refs

// Pattern for pickBlock area (near 0x0207fdcf)
static uint8_t s_pickBlock_pattern[] = {
    0xFD, 0x7B, 0xBF, 0xA9,  // stp x29, x30, [sp, #-0x10]!
    0xFD, 0x03, 0x00, 0x91,  // mov x29, sp
    // ... more bytes would go here
};

// Scan memory for pattern
void* scanPattern(void* start, size_t size, uint8_t* pattern, size_t patternLen) {
    uint8_t* data = (uint8_t*)start;
    for (size_t i = 0; i < size - patternLen; i++) {
        if (memcmp(data + i, pattern, patternLen) == 0) {
            return data + i;
        }
    }
    return nullptr;
}

// Get Minecraft base address
bool getMinecraftModule() {
    Dl_info info;
    if (dladdr((void*)&JNI_OnLoad, &info) == 0) {
        LOGE("dladdr failed");
        return false;
    }
    
    // Walk through loaded modules
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        LOGE("Failed to open /proc/self/maps");
        return false;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libminecraftpe.so")) {
            LOGI("Found: %s", line);
            // Parse line: start-end perms offset dev inode pathname
            unsigned long start, end;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                if (!g_mcBase) {
                    g_mcBase = (void*)start;
                    g_mcSize = end - start;
                    LOGI("Minecraft base: %p, size: %zu", g_mcBase, g_mcSize);
                }
            }
        }
    }
    fclose(fp);
    return g_mcBase != nullptr;
}

// Hook function using PLT/GOT or direct patch
bool hookFunction(void* target, void* replacement, void** original) {
    // Simple hook: just log for now
    LOGI("Hook target: %p -> %p", target, replacement);
    *original = target;
    return true;
}

// Our replacement render function
void hooked_render() {
    LOGI("Render hook called!");
    // Call original
    // Draw our overlay here
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("BlockOverlay loading...");
    
    // Get Minecraft module info
    if (!getMinecraftModule()) {
        LOGE("Failed to find Minecraft module");
        return JNI_VERSION_1_6;
    }
    
    // Calculate actual addresses from our string refs
    // String refs we found:
    // pickBlock: 0x0207fdcf -> function likely nearby
    // clip: 0x01f9a9a9
    // getBlock: 0x0063bbb7
    // HitResult: 0x01ff9196
    
    // For now, just log what we found
    LOGI("String references:");
    LOGI("  pickBlock @ %p", (char*)g_mcBase + 0x0207fdcf);
    LOGI("  clip @ %p", (char*)g_mcBase + 0x01f9a9a9);
    LOGI("  getBlock @ %p", (char*)g_mcBase + 0x0063bbb7);
    LOGI("  HitResult @ %p", (char*)g_mcBase + 0x01ff9196);
    
    // TODO: Scan backwards from strings to find function starts
    // TODO: Hook the render function
    
    LOGI("BlockOverlay loaded - ready for hooks");
    return JNI_VERSION_1_6;
}

// Export functions for manual hooking from external tools
extern "C" {
    __attribute__((visibility("default")))
    void overlay_set_color(float r, float g, float b, float a) {
        g_color[0] = r; g_color[1] = g; g_color[2] = b; g_color[3] = a;
        LOGI("Color: %f,%f,%f,%f", r, g, b, a);
    }
    
    __attribute__((visibility("default")))
    void overlay_draw(float x, float y, float z, int face) {
        // This would be called from hooked render function
        LOGI("Draw at %f,%f,%f face %d", x, y, z, face);
    }
}
