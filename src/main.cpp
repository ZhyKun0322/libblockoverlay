#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cstdio>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "BlockOverlay", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "BlockOverlay", __VA_ARGS__)

// Color config - BLUE with 30% transparency
static float g_color[4] = {0.0f, 0.4f, 1.0f, 0.3f};

// Hook original eglSwapBuffers
static EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;

// Simple overlay drawing function
static void drawOverlay() {
    // Save GL state
    GLboolean blend = glIsEnabled(GL_BLEND);
    GLint lastBlendSrc, lastBlendDst;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &lastBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &lastBlendDst);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth test so overlay appears on top
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    
    // Set up orthographic projection (2D drawing)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0, 1, 0, 1, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Draw blue quad in center of screen (test)
    // In real implementation, this would be at the targeted block position
    float r = g_color[0], g = g_color[1], b = g_color[2], a = g_color[3];
    
    glBegin(GL_QUADS);
    glColor4f(r, g, b, a);
    glVertex2f(0.4f, 0.4f);  // Bottom-left
    glVertex2f(0.6f, 0.4f);  // Bottom-right
    glVertex2f(0.6f, 0.6f);  // Top-right
    glVertex2f(0.4f, 0.6f);  // Top-left
    glEnd();
    
    // Restore GL state
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    if (depth) glEnable(GL_DEPTH_TEST);
    if (!blend) glDisable(GL_BLEND);
    glBlendFunc(lastBlendSrc, lastBlendDst);
}

// Hooked eglSwapBuffers - called every frame!
static EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // Draw our overlay before swapping buffers
    drawOverlay();
    
    // Call original function
    return orig_eglSwapBuffers(dpy, surface);
}

// Get library address
void* getLibraryAddress(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return nullptr;
    
    char line[512];
    void* addr = nullptr;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, name) && strstr(line, "r-xp")) {
            unsigned long start;
            if (sscanf(line, "%lx-", &start) == 1) {
                addr = (void*)start;
                break;
            }
        }
    }
    fclose(fp);
    return addr;
}

// Simple PLT hook - replace function pointer in GOT/PLT
bool hookFunction(const char* libName, const char* symbol, void* newFunc, void** oldFunc) {
    void* libAddr = getLibraryAddress(libName);
    if (!libAddr) {
        LOGE("Failed to find %s", libName);
        return false;
    }
    
    // Get symbol address using dlsym
    void* symAddr = dlsym(RTLD_DEFAULT, symbol);
    if (!symAddr) {
        LOGE("Failed to find symbol %s", symbol);
        return false;
    }
    
    *oldFunc = symAddr;
    
    // Replace function pointer (simplified - real implementation needs memory protection)
    // For now, just log that we found it
    LOGI("Found %s at %p, would hook to %p", symbol, symAddr, newFunc);
    
    return true;
}

// Initialize hooks
void initHooks() {
    LOGI("Initializing hooks...");
    
    // Hook eglSwapBuffers to draw overlay every frame
    if (hookFunction("libEGL.so", "eglSwapBuffers", (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers)) {
        LOGI("eglSwapBuffers hook ready");
    }
}

// Constructor - runs when .so is loaded!
__attribute__((constructor))
void onModLoad() {
    LOGI("========================================");
    LOGI("BlockOverlay MOD LOADED!");
    LOGI("========================================");
    
    initHooks();
    
    LOGI("BlockOverlay initialization complete");
}

// Also export JNI_OnLoad for compatibility
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("BlockOverlay JNI_OnLoad called");
    return JNI_VERSION_1_6;
}

// Export functions for configuration
extern "C" {
    __attribute__((visibility("default")))
    void overlay_set_color(float r, float g, float b, float a) {
        g_color[0] = r; g_color[1] = g; g_color[2] = b; g_color[3] = a;
        LOGI("Color: %f,%f,%f,%f", r, g, b, a);
    }
}
