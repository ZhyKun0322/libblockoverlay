#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <cmath>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "BlockOverlay", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "BlockOverlay", __VA_ARGS__)

// Blue color with 30% transparency
static float g_color[4] = {0.0f, 0.4f, 1.0f, 0.3f};

// Target block info
static float g_targetX = 0.0f;
static float g_targetY = 0.0f;
static float g_targetZ = 0.0f;
static int g_targetFace = 1; // 0=down, 1=up, 2=north, 3=south, 4=west, 5=east
static bool g_hasTarget = false;
static JavaVM* g_vm = nullptr;

// Export functions for Minecraft hooks to call
extern "C" {

// Update target block position (called from Minecraft hook)
__attribute__((visibility("default")))
void overlay_set_target(float x, float y, float z, int face) {
    g_targetX = x;
    g_targetY = y;
    g_targetZ = z;
    g_targetFace = face;
    g_hasTarget = true;
}

// Clear target (when not looking at block)
__attribute__((visibility("default")))
void overlay_clear_target() {
    g_hasTarget = false;
}

// Get vertices for the overlay quad
// Returns 12 floats: 4 vertices * 3 coords (x,y,z)
__attribute__((visibility("default")))
void overlay_get_vertices(float* out_vertices, float* out_colors) {
    if (!g_hasTarget) return;
    
    float x = g_targetX;
    float y = g_targetY;
    float z = g_targetZ;
    float offset = 0.002f; // Slightly above block to avoid z-fighting
    
    // 4 vertices for quad
    float v[12];
    
    switch(g_targetFace) {
        case 0: // Down (Y-)
            v[0]=x;     v[1]=y-offset; v[2]=z;
            v[3]=x+1.0; v[4]=y-offset; v[5]=z;
            v[6]=x+1.0; v[7]=y-offset; v[8]=z+1.0;
            v[9]=x;     v[10]=y-offset; v[11]=z+1.0;
            break;
        case 1: // Up (Y+) - Most common
            v[0]=x;     v[1]=y+1.0+offset; v[2]=z;
            v[3]=x+1.0; v[4]=y+1.0+offset; v[5]=z;
            v[6]=x+1.0; v[7]=y+1.0+offset; v[8]=z+1.0;
            v[9]=x;     v[10]=y+1.0+offset; v[11]=z+1.0;
            break;
        case 2: // North (Z-)
            v[0]=x;     v[1]=y;     v[2]=z-offset;
            v[3]=x+1.0; v[4]=y;     v[5]=z-offset;
            v[6]=x+1.0; v[7]=y+1.0; v[8]=z-offset;
            v[9]=x;     v[10]=y+1.0; v[11]=z-offset;
            break;
        case 3: // South (Z+)
            v[0]=x;     v[1]=y;     v[2]=z+1.0+offset;
            v[3]=x+1.0; v[4]=y;     v[5]=z+1.0+offset;
            v[6]=x+1.0; v[7]=y+1.0; v[8]=z+1.0+offset;
            v[9]=x;     v[10]=y+1.0; v[11]=z+1.0+offset;
            break;
        case 4: // West (X-)
            v[0]=x-offset; v[1]=y;     v[2]=z;
            v[3]=x-offset; v[4]=y;     v[5]=z+1.0;
            v[6]=x-offset; v[7]=y+1.0; v[8]=z+1.0;
            v[9]=x-offset; v[10]=y+1.0; v[11]=z;
            break;
        case 5: // East (X+)
            v[0]=x+1.0+offset; v[1]=y;     v[2]=z;
            v[3]=x+1.0+offset; v[4]=y;     v[5]=z+1.0;
            v[6]=x+1.0+offset; v[7]=y+1.0; v[8]=z+1.0;
            v[9]=x+1.0+offset; v[10]=y+1.0; v[11]=z;
            break;
    }
    
    // Copy vertices
    memcpy(out_vertices, v, 12 * sizeof(float));
    
    // Copy colors (RGBA for each vertex)
    for (int i = 0; i < 4; i++) {
        out_colors[i*4+0] = g_color[0];
        out_colors[i*4+1] = g_color[1];
        out_colors[i*4+2] = g_color[2];
        out_colors[i*4+3] = g_color[3];
    }
}

// Change color
__attribute__((visibility("default")))
void overlay_set_color(float r, float g, float b, float a) {
    g_color[0] = r;
    g_color[1] = g;
    g_color[2] = b;
    g_color[3] = a;
    LOGI("Color changed to %f,%f,%f,%f", r, g, b, a);
}

} // extern "C"

// JNI_OnLoad - THIS IS CALLED by LeviLaunchroid when loading the .so!
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    LOGI("BlockOverlay mod loaded via JNI_OnLoad!");
    LOGI("Blue overlay ready - RGBA: 0.0, 0.4, 1.0, 0.3");
    LOGI("Export functions: overlay_set_target, overlay_get_vertices, overlay_set_color");
    
    // Return JNI version
    return JNI_VERSION_1_6;
}
