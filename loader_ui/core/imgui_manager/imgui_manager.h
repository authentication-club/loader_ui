#ifndef IMGUI_MANAGER_HPP
#define IMGUI_MANAGER_HPP

#include <d3d11.h>
#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>
#include "../dep/imgui/imgui.h"
#include "../dep/imgui/imgui_impl_win32.h"
#include "../dep/imgui/imgui_impl_dx11.h"

struct font_object {
    ImFont* font;
    const char* name;
    const char* font_path;
    float size;

    font_object(ImFont* f, const char* n, const char* path, float s)
        : font(f), name(n), font_path(path), size(s) {
    }
};

static ImColor darken(const ImColor& color, float factor = 0.8f) {
    auto clamp = [](float x) { return std::max<float>(0.0f, std::min<float>(1.0f, x)); };
    return ImColor(
        clamp(color.Value.x * factor),
        clamp(color.Value.y * factor),
        clamp(color.Value.z * factor),
        color.Value.w
    );
}

class c_imgui_manager {
private:
    HWND hwnd;
    ID3D11Device* pd3dDevice;
    ID3D11DeviceContext* pd3dDeviceContext;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pMainRenderTargetView;
    std::vector<font_object*> fonts;
    bool initialized;

    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    c_imgui_manager();
    ~c_imgui_manager();

    bool initialize(const std::string& title);
    ImFont* get_font(const char* font_name);
    void initalize_fonts();
    void shutdown();
    bool should_close() const;
    void new_frame();
    void render();
    void present();

    HWND get_hwnd() const { return hwnd; }
    void set_should_close(bool close);

    // Utility functions
    void set_window_title(const std::string& title);
};
#endif // IMGUI_MANAGER_HPP
