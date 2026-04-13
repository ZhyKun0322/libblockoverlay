#include <dobby.h>
#include <android/log.h>

#define LOG_TAG "BlockOverlay"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct mceColor {
    float r, g, b, a;
};

// Original function pointer
void (*orig_BlockSelectionRenderer_render)(void* self, void* screenContext, void* levelRenderer, void* blockPos, const mceColor& color, float thickness);

// Our Hooked Function
void hook_BlockSelectionRenderer_render(void* self, void* screenContext, void* levelRenderer, void* blockPos, const mceColor& color, float thickness) {
    
    // Create our custom blue color
    mceColor debugBlue = { 0.0f, 0.0f, 1.0f, 1.0f };
    
    // Set thickness (Standard thin line)
    float customThickness = 0.04f; 

    // Call the original game function but pass our blue color and thickness
    if (orig_BlockSelectionRenderer_render) {
        orig_BlockSelectionRenderer_render(self, screenContext, levelRenderer, blockPos, debugBlue, customThickness);
    }
}

// Entry point when the mod loads
void __attribute__((constructor)) init() {
    LOGI("Mod loading...");

    // DobbyHook is used here. 
    // Note: The symbol below is for a specific version. 
    // If it doesn't work, you might need the hex offset for your specific libminecraftpe.so
    DobbyHook(
        (void*)DobbySymbolResolver("libminecraftpe.so", "_ZN22BlockSelectionRenderer6renderER13ScreenContextR13LevelRendererRK8BlockPosRK3mce5Colorf"),
        (void*)hook_BlockSelectionRenderer_render,
        (void**)&orig_BlockSelectionRenderer_render
    );

    LOGI("Mod hooked successfully!");
}
