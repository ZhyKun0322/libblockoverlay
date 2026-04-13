#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cstdio>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "BlockOverlay", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "BlockOverlay", __VA_ARGS__)

// Color config - BLUE with 30% transparency
static float g_color[4] = {0.0f, 0.4f, 1.0f, 0.3f};

// Module base address
static void* g_mcBase = nullptr;
static size_t g_mcSize = 0;

// Get Minecraft base address
bool getMinecraftModule() {
    // Walk through loaded modules
    std::FILE* fp = std::fopen("/proc/self/maps", "r");
    if (!fp) {
        LOGE("Failed to open /proc/self/maps");
        return false;
    }
    
    char line[512];
    while (std::fgets(line, sizeof(line), fp)) {
        if (std::strstr(line, "libminecraftpe.so")) {
            LOGI("Found: %s", line);
            // Parse line: start-end perms offset dev inode pathname
            unsigned long start, end;
            if (std::sscanf(line, "%lx-%lx", &start, &end) == 2) {
                if (!g_mcBase) {
                    g_mcBase = (void*)start;
                    g_mcSize = end - start;
                    LOGI("Minecraft base: %p, size: %zu", g_mcBase, g_mcSize);
                    break;  // Just get first occurrence
                }
            }
        }
    }
    std::fclose(fp);
    return g_mcBase != nullptr;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("BlockOverlay loading...");
    
    // Get Minecraft module info
    if (!getMinecraftModule()) {
        LOGE("Failed to find Minecraft module");
        return JNI_VERSION_1_6;
    }
    
    // Calculate actual addresses from our string refs
    LOGI("String references:");
    LOGI("  pickBlock @ %p", (char*)g_mcBase + 0x0207fdcf);
    LOGI("  clip @ %p", (char*)g_mcBase + 0x01f9a9a9);
    LOGI("  getBlock @ %p", (char*)g_mcBase + 0x0063bbb7);
    LOGI("  HitResult @ %p", (char*)g_mcBase + 0x01ff9196);
    
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
        LOGI("Draw at %f,%f,%f face %d", x, y, z, face);
    }
}
