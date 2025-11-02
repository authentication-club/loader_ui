#define IMGUI_DEFINE_MATH_OPERATORS
#include "loader_ui.h"
#include "../imgui_manager/imgui_manager.h"
#include "../dep/imgui/imgui.h"
#include <iostream>
#include <cstring>
#include <cstddef>
#include <array>
#include <algorithm>
#include <cctype>
#include <string_view>

static c_imgui_manager* imgui_manager = nullptr;

c_loader_ui::c_loader_ui()
    : should_close(false), initialized(false) {
}

c_loader_ui::~c_loader_ui() {
    shutdown();
}

bool c_loader_ui::initialize(const ui_config& cfg) {
    if (initialized) {
        return true;
    }

    config = cfg;

    if (!imgui_manager) {
        imgui_manager = new c_imgui_manager();
    }

    if (!imgui_manager->initialize(config.title)) {
        std::cerr << "Failed to initialize ImGui manager" << std::endl;
        return false;
    }

    apply_base_theme();

    initialized = true;
    std::cout << "UI initialized successfully" << std::endl;
    return true;
}

void c_loader_ui::shutdown() {
    if (!initialized) {
        return;
    }

    state = ui_state{};

    if (imgui_manager) {
        imgui_manager->shutdown();
        delete imgui_manager;
        imgui_manager = nullptr;
    }

    initialized = false;
}

bool c_loader_ui::should_run() const {
    if (!initialized || !imgui_manager) {
        return false;
    }

    return !should_close && !imgui_manager->should_close();
}

void c_loader_ui::update() {
    if (!initialized || !imgui_manager) {
        return;
    }

    imgui_manager->new_frame();
}

void c_loader_ui::render() {
    if (!initialized || !imgui_manager) {
        return;
    }

    render_auth_mode_window();

    if (state.show_login_window) {
        render_login_window();
    }

    if (state.show_register_window) {
        render_register_window();
    }

    if (state.show_main_window) {
        render_main_window();
    }

    imgui_manager->render();
    imgui_manager->present();
}

void c_loader_ui::render_auth_mode_window() {
    if (!initialized || !imgui_manager) {
        return;
    }

    ImVec2 window_pos((float)GetSystemMetrics(SM_CXSCREEN) - 10.0f,
        10.0f);
    ImVec2 pivot(1.0f, 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::SetNextWindowSize(ImVec2(220.0f, 70.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Authentication Mode##auth_mode_window",
        nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize)) {

        bool mode = state.license_only_mode;
        if (ImGui::Checkbox("License-only login", &mode)) {
            state.license_only_mode = mode;
            if (auth_mode_callback) {
                auth_mode_callback(mode);
            }

            if (mode) {
                if (!state.authenticated) {
                    show_main();
                }
            }
            else {
                if (!state.authenticated) {
                    show_login();
                }
            }
        }

        ImGui::Spacing();
        ImGui::TextUnformatted(mode
            ? "Uses stored local account"
            : "Manual username/password");
    }
    ImGui::End();
}

void c_loader_ui::render_login_window() {
    ImGui::SetNextWindowPos(ImVec2((float)GetSystemMetrics(SM_CXSCREEN) / 2, (float)GetSystemMetrics(SM_CYSCREEN) / 2),
        ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Bootstrapper##login window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGuiIO& io = ImGui::GetIO();
        auto colors = ImGui::GetStyle().Colors;

        ImGui::BeginChild("##header", ImVec2(ImGui::GetWindowWidth() - 30, 35));
        {
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(),
                ImColor(colors[ImGuiCol_ChildBg]), ImColor(colors[ImGuiCol_ChildBg]),
                darken(ImColor(colors[ImGuiCol_ChildBg])), darken(ImColor(colors[ImGuiCol_ChildBg])));

            ImGui::PushFont(imgui_manager->get_font("title"));
            ImVec2 title_size = ImGui::CalcTextSize("Welcome Back");
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - title_size.x / 2, title_size.y / 8));
            ImGui::Text("Welcome Back");
            ImGui::SameLine(5);
            if (ImGui::Button("X", ImVec2(25, 25))) {
                if (exit_callback) {
                    exit_callback();
                }
                should_close = true;
            }
            ImGui::PopFont();

            ImGui::EndChild();
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::BeginChild("##body", ImVec2(ImGui::GetWindowWidth() - 30, ImGui::GetWindowHeight() - 80));
        {
            ImGui::PushFont(imgui_manager->get_font("smalltitle"));
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25));
            static char username_buffer[256] = "";
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowWidth() / 8, 15));
            ImGui::InputTextWithHint("##username", "Login", username_buffer, sizeof(username_buffer));

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25));
            static char password_buffer[256] = "";
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 8);
            ImGui::InputTextWithHint("##password", "Password", password_buffer, sizeof(password_buffer), ImGuiInputTextFlags_Password);
            ImGui::PopFont();

            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowWidth() / 8, ImGui::GetWindowHeight() / 2.5));
            if (ImGui::Button("Login", ImVec2(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25), 35))) {
                if (login_callback && strlen(username_buffer) > 0 && strlen(password_buffer) > 0) {
                    login_callback(std::string(username_buffer), std::string(password_buffer));
                }
            }
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 8);
            if (ImGui::Button("Register", ImVec2(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25), 35))) {
                show_register();
            }

            if (!state.status_message.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 0.7f, 1.0f));
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(state.status_message.c_str()).x / 2);
                ImGui::Text(state.status_message.c_str());
                ImGui::PopStyleColor();
            }

            if (!state.error_message.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(state.status_message.c_str()).x / 2);
                ImGui::Text("%s", state.error_message.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }
}

void c_loader_ui::render_register_window() {
    ImGui::SetNextWindowPos(ImVec2((float)GetSystemMetrics(SM_CXSCREEN) / 2, (float)GetSystemMetrics(SM_CYSCREEN) / 2),
        ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Bootstrapper##register window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGuiIO& io = ImGui::GetIO();
        auto colors = ImGui::GetStyle().Colors;

        ImGui::BeginChild("##header", ImVec2(ImGui::GetWindowWidth() - 30, 35));
        {
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(),
                ImColor(colors[ImGuiCol_ChildBg]), ImColor(colors[ImGuiCol_ChildBg]),
                darken(ImColor(colors[ImGuiCol_ChildBg])), darken(ImColor(colors[ImGuiCol_ChildBg])));

            ImGui::PushFont(imgui_manager->get_font("title"));
            ImVec2 title_size = ImGui::CalcTextSize("Join Us Today");
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - title_size.x / 2, title_size.y / 8));
            ImGui::Text("Join Us Today");
            ImGui::SameLine(5);
            if (ImGui::Button("X", ImVec2(25, 25))) {
                if (exit_callback) {
                    exit_callback();
                }
                should_close = true;
            }
            ImGui::PopFont();

            ImGui::EndChild();
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::BeginChild("##body", ImVec2(ImGui::GetWindowWidth() - 30, ImGui::GetWindowHeight() - 80));
        {
            ImGui::PushFont(imgui_manager->get_font("smalltitle"));
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25));
            static char username_buffer[256] = "";
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowWidth() / 8, 15));
            ImGui::InputTextWithHint("##username", "Login", username_buffer, sizeof(username_buffer));

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25));
            static char password_buffer[256] = "";
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 8);
            ImGui::InputTextWithHint("##password", "Password", password_buffer, sizeof(password_buffer), ImGuiInputTextFlags_Password);

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25));
            static char license_buffer[256] = "";
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 8);
            ImGui::InputTextWithHint("##license", "License", license_buffer, sizeof(license_buffer));
            ImGui::PopFont();

            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowWidth() / 8, ImGui::GetWindowHeight() / 3.5));
            if (ImGui::Button("Create Account", ImVec2(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25), 35))) {
                if (register_callback && strlen(username_buffer) > 0 &&
                    strlen(password_buffer) > 0 && strlen(license_buffer) > 0) {
                    register_callback(std::string(username_buffer),
                        std::string(password_buffer),
                        std::string(license_buffer));
                }
            }
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 8);
            if (ImGui::Button("Back To Login", ImVec2(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() * 0.25), 35))) {
                show_login();
            }

            if (!state.status_message.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 0.7f, 1.0f));
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(state.status_message.c_str()).x / 2);
                ImGui::Text(state.status_message.c_str());
                ImGui::PopStyleColor();
            }

            if (!state.error_message.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(state.status_message.c_str()).x / 2);
                ImGui::Text("%s", state.error_message.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }
}

void c_loader_ui::render_main_window() {
    ImGui::SetNextWindowPos(ImVec2((float)GetSystemMetrics(SM_CXSCREEN) / 2, (float)GetSystemMetrics(SM_CYSCREEN) / 2),
        ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("Bootstrapper##main window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGuiIO& io = ImGui::GetIO();
        auto colors = ImGui::GetStyle().Colors;

        ImGui::BeginChild("##header", ImVec2(ImGui::GetWindowWidth() - 30, 45));
        {
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(),
                ImColor(colors[ImGuiCol_ChildBg]), ImColor(colors[ImGuiCol_ChildBg]),
                darken(ImColor(colors[ImGuiCol_ChildBg])), darken(ImColor(colors[ImGuiCol_ChildBg])));

            ImGui::PushFont(imgui_manager->get_font("title"));
            int title_height = ImGui::CalcTextSize(config.application_name).y;
            ImGui::SetCursorPos(ImVec2(5, title_height / 7));
            ImGui::Text(config.application_name);
            ImGui::PopFont();

            ImGui::PushFont(imgui_manager->get_font("subtitle"));
            ImGui::SetCursorPos(ImVec2(15, ImGui::GetCursorPosY() -
                ImGui::CalcTextSize(user.username.c_str()).y / 1.8f));

            ImGui::Text("Welcome %s", user.username.c_str());
            ImGui::PopFont();
            ImGui::EndChild();
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::BeginChild("##body", ImVec2(ImGui::GetWindowWidth() - 30, ImGui::GetWindowHeight() - 118));
        {

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
            for (auto sub : user.subscriptions) {
                ImGui::BeginChild(sub->plan.c_str(), ImVec2(ImGui::GetWindowWidth(), 40), true,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(),
                    ImColor(colors[ImGuiCol_ChildBg]), ImColor(colors[ImGuiCol_ChildBg]),
                    darken(ImColor(colors[ImGuiCol_ChildBg]), 0.5f), darken(ImColor(colors[ImGuiCol_ChildBg]), 0.5f));

                ImGui::PushFont(imgui_manager->get_font("subtitle"));
                float height_from_bottom = ImGui::CalcTextSize("Expires at: %s Status: %s").y + 5;
                ImGui::SetCursorPos(ImVec2(10, ImGui::GetWindowHeight() - height_from_bottom));
                ImGui::Text("Expires at: %s", sub->expires_at.c_str());
                ImGui::SameLine();
                ImGui::Text(" Status: %s", sub->status.c_str());
                ImGui::PopFont();

                ImGui::PushFont(imgui_manager->get_font("smalltitle"));
                ImGui::SetCursorPos(ImVec2(10, ImGui::GetWindowHeight() - height_from_bottom -
                    ImGui::CalcTextSize(sub->plan.c_str()).y));
                ImGui::Text("%s", sub->plan.c_str());
                ImGui::PopFont();

                ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 105, 5));
                if (ImGui::Button("Launch", ImVec2(100, ImGui::GetWindowHeight() - 10)))
                {
                    if (filestream_callback && sub->plan == "Rust" && sub->status == "Active")
                    {
                        filestream_callback("4676609091914915"); // use correct id
                    }
                }
                ImGui::EndChild();
            }
            ImGui::PopStyleVar();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::EndChild();
        }

        ImGui::BeginChild("##redeembody", ImVec2(ImGui::GetWindowWidth() - 30, 25));
        {
            static char license_buffer[128];
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.75f);
            ImGui::InputTextWithHint("##license", "License", license_buffer, IM_ARRAYSIZE(license_buffer));
            ImGui::SameLine();
            if (ImGui::Button("Redeem", ImVec2(ImGui::GetWindowWidth() * 0.225f, 25))) {
                if (license_callback && !user.username.empty() && strlen(license_buffer) > 0) {
                    license_callback(user.username, std::string(license_buffer));
                }
            }
            ImGui::EndChild();
        }

        ImGui::End();
    }
}

void c_loader_ui::apply_base_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.90f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.80f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarSize = 2.f;
    style.ScrollbarRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 8);
    style.Alpha = 0.95f;
}

void c_loader_ui::set_authenticated(bool auth, user_profile* new_profile) {
    state.authenticated = auth;

    if (new_profile != nullptr) {
        user = *new_profile;
    }

    if (state.authenticated) {
        products_dirty = true;
        show_main();
    }
    else {
        release_product_views();
        products_dirty = false;
        user.subscriptions.clear();
        if (state.license_only_mode) {
            state.show_login_window = false;
            state.show_register_window = false;
            state.show_main_window = true;
        }
        else {
            show_login();
        }
    }
}


void c_loader_ui::set_status_message(const std::string& message) {
    state.status_message = message;
    state.error_message.clear();
}

void c_loader_ui::set_error_message(const std::string& message) {
    state.error_message = message;
    state.status_message.clear();
}

void c_loader_ui::set_loading(bool active) {

}

void c_loader_ui::show_login() {
    if (state.license_only_mode) {
        state.show_login_window = false;
        state.show_register_window = false;
        state.show_main_window = true;
    }
    else {
        state.show_login_window = true;
        state.show_register_window = false;
        state.show_main_window = false;
    }
}

void c_loader_ui::show_register() {
    if (state.license_only_mode) {
        state.show_login_window = false;
        state.show_register_window = false;
        state.show_main_window = true;
    }
    else {
        state.show_login_window = false;
        state.show_register_window = true;
        state.show_main_window = false;
    }
}

void c_loader_ui::show_main() {
    state.show_login_window = false;
    state.show_register_window = false;
    state.show_main_window = true;
}

void c_loader_ui::set_local_account_username(const std::string& username) {
    user.username = username;
}

void c_loader_ui::set_license_only_mode(bool enabled) {
    state.license_only_mode = enabled;
    if (enabled) {
        if (!state.authenticated) {
            show_main();
        }
    }
    else {
        if (!state.authenticated) {
            show_login();
        }
    }
}

void c_loader_ui::set_login_callback(LoginCallback callback) {
    login_callback = callback;
}

void c_loader_ui::set_register_callback(RegisterCallback callback) {
    register_callback = callback;
}

void c_loader_ui::set_license_callback(LicenseCallback callback) {
    license_callback = callback;
}

void c_loader_ui::set_exit_callback(ExitCallback callback) {
    exit_callback = callback;
}

void c_loader_ui::set_filestream_callback(FilestreamCallback callback) {
    filestream_callback = callback;
}

void c_loader_ui::set_auth_mode_callback(AuthModeCallback callback) {
    auth_mode_callback = callback;
}

void c_loader_ui::handle_login_request(const std::string& username, const std::string& password) {
    if (login_callback) {
        login_callback(username, password);
    }
}

void c_loader_ui::handle_register_request(const std::string& username, const std::string& password, const std::string& license) {
    if (register_callback) {
        register_callback(username, password, license);
    }
}

void c_loader_ui::handle_launch_request(const std::string& file_id) {
    if (filestream_callback && !file_id.empty()) {
        filestream_callback(file_id);
    }
}

void c_loader_ui::handle_license_redeem(const std::string& license) {
    if (license_callback && !user.username.empty() && !license.empty()) {
        license_callback(user.username, license);
    }
}

const std::vector<c_loader_ui::product_view>& c_loader_ui::get_product_views() const {
    return products;
}

void c_loader_ui::release_product_views() {

}

void c_loader_ui::initialize_fallback_icons() {

}

void c_loader_ui::release_fallback_icons() {
    auto release_icon = [&](ID3D11ShaderResourceView*& slot,
        ID3D11ShaderResourceView*& fallback_slot,
        std::string_view name) {
            if (!fallback_slot) {
                return;
            }

            if (slot == fallback_slot) {
                slot = nullptr;
            }

            fallback_slot->Release();
            fallback_slot = nullptr;
        };
}

void c_loader_ui::rebuild_product_views() {
}

void c_loader_ui::close() {
    should_close = true;
}

static void(*g_login_callback)(const char*, const char*) = nullptr;
static void(*g_register_callback)(const char*, const char*, const char*) = nullptr;
static void(*g_license_callback)(const char*, const char*) = nullptr;
static void(*g_exit_callback)() = nullptr;
static void (*g_filestream_callback)(const char*) = nullptr;
static void(*g_auth_mode_callback)(bool) = nullptr;

extern "C" {
    LOADER_UI_API c_loader_ui* create_loader_ui() {
        return new c_loader_ui();
    }

    LOADER_UI_API void destroy_loader_ui(c_loader_ui* ui) {
        delete ui;
    }

    LOADER_UI_API bool ui_initialize(c_loader_ui* ui, const char* title) {
        if (!ui) return false;

        ui_config config;
        config.title = title ? title : "Bootstrapper";

        return ui->initialize(config);
    }

    LOADER_UI_API void ui_shutdown(c_loader_ui* ui) {
        if (ui) ui->shutdown();
    }

    LOADER_UI_API bool ui_should_run(c_loader_ui* ui) {
        return ui ? ui->should_run() : false;
    }

    LOADER_UI_API void ui_update(c_loader_ui* ui) {
        if (ui) ui->update();
    }

    LOADER_UI_API void ui_render(c_loader_ui* ui) {
        if (ui) ui->render();
    }

    LOADER_UI_API void ui_set_authenticated(c_loader_ui* ui, bool auth, user_profile* new_profile) {
        if (ui) {
            ui->set_authenticated(auth, new_profile);
        }
    }

    LOADER_UI_API void ui_set_status_message(c_loader_ui* ui, const char* message) {
        if (ui) ui->set_status_message(message ? message : "");
    }

    LOADER_UI_API void ui_set_error_message(c_loader_ui* ui, const char* message) {
        if (ui) ui->set_error_message(message ? message : "");
    }

    LOADER_UI_API void ui_set_loading(c_loader_ui* ui, bool loading) {
        if (ui) ui->set_loading(loading);
    }

    LOADER_UI_API void ui_close(c_loader_ui* ui) {
        if (ui) ui->close();
    }

    LOADER_UI_API void ui_set_local_account(c_loader_ui* ui, const char* username) {
        if (ui && username) {
            ui->set_local_account_username(username);
        }
    }

    LOADER_UI_API void ui_set_license_only_mode(c_loader_ui* ui, bool enabled) {
        if (ui) {
            ui->set_license_only_mode(enabled);
        }
    }

    LOADER_UI_API void ui_set_auth_mode_callback(c_loader_ui* ui, void(*callback)(bool)) {
        if (!ui) return;

        g_auth_mode_callback = callback;

        if (callback) {
            ui->set_auth_mode_callback([](bool enabled) {
                if (g_auth_mode_callback) {
                    g_auth_mode_callback(enabled);
                }
                });
        }
        else {
            ui->set_auth_mode_callback(nullptr);
        }
    }

    LOADER_UI_API void ui_set_login_callback(c_loader_ui* ui, void(*callback)(const char*, const char*)) {
        if (!ui) return;

        g_login_callback = callback;

        if (callback) {
            ui->set_login_callback([](const std::string& username, const std::string& password) {
                if (g_login_callback) {
                    g_login_callback(username.c_str(), password.c_str());
                }
                });
        }
        else {
            ui->set_login_callback(nullptr);
        }
    }

    LOADER_UI_API void ui_set_register_callback(c_loader_ui* ui, void(*callback)(const char*, const char*, const char*)) {
        if (!ui) return;

        g_register_callback = callback;

        if (callback) {
            ui->set_register_callback([](const std::string& username, const std::string& password, const std::string& license) {
                if (g_register_callback) {
                    g_register_callback(username.c_str(), password.c_str(), license.c_str());
                }
                });
        }
        else {
            ui->set_register_callback(nullptr);
        }
    }

    LOADER_UI_API void ui_set_license_callback(c_loader_ui* ui, void(*callback)(const char*, const char*)) {
        if (!ui) return;

        g_license_callback = callback;

        if (callback) {
            ui->set_license_callback([](const std::string& username, const std::string& license) {
                if (g_license_callback) {
                    g_license_callback(username.c_str(), license.c_str());
                }
                });
        }
        else {
            ui->set_license_callback(nullptr);
        }
    }

    LOADER_UI_API void ui_set_exit_callback(c_loader_ui* ui, void(*callback)()) {
        if (!ui) return;

        g_exit_callback = callback;

        if (callback) {
            ui->set_exit_callback([]() {
                if (g_exit_callback) {
                    g_exit_callback();
                }
                });
        }
        else {
            ui->set_exit_callback(nullptr);
        }
    }


    LOADER_UI_API void ui_set_filestream_callback(
        c_loader_ui* ui,
        void(*callback)(const char*)
    ) {
        if (!ui) return;

        g_filestream_callback = callback;

        if (callback) {
            ui->set_filestream_callback([](const std::string& file_id)
                {
                    if (g_filestream_callback) {
                        g_filestream_callback(file_id.c_str());
                    }
                });
        }
        else {
            ui->set_filestream_callback(nullptr);
        }
    }
}
