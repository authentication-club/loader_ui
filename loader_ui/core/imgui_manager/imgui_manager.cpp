#include "imgui_manager.h"
#include <iostream>
#include <tchar.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool g_should_close = false;

c_imgui_manager::c_imgui_manager()
    : hwnd(nullptr), pd3dDevice(nullptr), pd3dDeviceContext(nullptr),
    pSwapChain(nullptr), pMainRenderTargetView(nullptr), initialized(false) {
}

c_imgui_manager::~c_imgui_manager() {
    shutdown();
}

LRESULT WINAPI c_imgui_manager::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_DESTROY:
        g_should_close = true;
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top,
                suggested_rect->right - suggested_rect->left,
                suggested_rect->bottom - suggested_rect->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool c_imgui_manager::initialize(const std::string& title) {
    if (initialized) {
        return true;
    }

    g_should_close = false;

    WNDCLASSEXW wc = {
            sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
            GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
            L"ImGui Context", nullptr
    };
    ::RegisterClassExW(&wc);

    hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"Hidden Context",
        WS_POPUP,
        -32000, -32000,
        1, 1,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = "";

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 12.0f;
        style.Colors[ImGuiCol_WindowBg].w = 0.95f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

    initalize_fonts();

    initialized = true;
    std::cout << "ImGui manager initialized" << std::endl;
    return true;
}

ImFont* c_imgui_manager::get_font(const char* font_name) {
    for (const auto& font : fonts) {
        if (font && strcmp(font->name, font_name) == 0)
            return font->font;
    }

    return nullptr;
}

void c_imgui_manager::initalize_fonts() {
    ImGuiIO& io = ImGui::GetIO();

    //First font pushed is default font
    fonts.push_back(new font_object(nullptr, "normal", "c:\\Windows\\Fonts\\bahnschrift.ttf", 14.f));
    fonts.push_back(new font_object(nullptr, "title", "c:\\Windows\\Fonts\\bahnschrift.ttf", 22.f));
    fonts.push_back(new font_object(nullptr, "smalltitle", "c:\\Windows\\Fonts\\bahnschrift.ttf", 18.f));
    fonts.push_back(new font_object(nullptr, "subtitle", "c:\\Windows\\Fonts\\bahnschrift.ttf", 10.f));

    for (auto font : fonts) {
        font->font = io.Fonts->AddFontFromFileTTF(font->font_path, font->size, NULL, io.Fonts->GetGlyphRangesDefault());
    }
}

void c_imgui_manager::shutdown() {
    if (!initialized) {
        return;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();

    if (hwnd) {
        ::DestroyWindow(hwnd);
        hwnd = nullptr;
    }
    ::UnregisterClassW(L"ImGui Context", GetModuleHandle(nullptr));

    initialized = false;
}

bool c_imgui_manager::should_close() const {
    return g_should_close;
}

void c_imgui_manager::new_frame() {
    if (!initialized) return;

    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            g_should_close = true;
    }

    if (g_should_close)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void c_imgui_manager::render() {
    if (!initialized) return;

    ImGui::Render();

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pd3dDeviceContext->OMSetRenderTargets(1, &pMainRenderTargetView, nullptr);
    pd3dDeviceContext->ClearRenderTargetView(pMainRenderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void c_imgui_manager::present() {
    if (!initialized) return;
    pSwapChain->Present(1, 0);
}

void c_imgui_manager::set_should_close(bool close) {
    g_should_close = close;
}

void c_imgui_manager::set_window_title(const std::string& title) {
    if (hwnd) {
        std::wstring wtitle(title.begin(), title.end());
        ::SetWindowTextW(hwnd, wtitle.c_str());
    }
}

bool c_imgui_manager::CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 1;
    sd.BufferDesc.Height = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain,
        &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain,
            &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void c_imgui_manager::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
    if (pd3dDeviceContext) { pd3dDeviceContext->Release(); pd3dDeviceContext = nullptr; }
    if (pd3dDevice) { pd3dDevice->Release(); pd3dDevice = nullptr; }
}

void c_imgui_manager::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRenderTargetView);
        pBackBuffer->Release();
    }
}

void c_imgui_manager::CleanupRenderTarget() {
    if (pMainRenderTargetView) {
        pMainRenderTargetView->Release();
        pMainRenderTargetView = nullptr;
    }
}
