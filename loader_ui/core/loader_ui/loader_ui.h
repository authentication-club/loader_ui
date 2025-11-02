#ifndef LOADER_UI_HPP
#define LOADER_UI_HPP

#include <string>
#include <functional>
#include <array>
#include <vector>
#include <memory>
#include <filesystem>

// DLL export/import macros - respect static builds
#ifdef _WIN32
#if defined(LOADER_UI_STATIC)
#define LOADER_UI_API
#elif defined(LOADER_UI_EXPORTS)
#define LOADER_UI_API __declspec(dllexport)
#else
#define LOADER_UI_API __declspec(dllimport)
#endif
#else
#define LOADER_UI_API
#endif

// Forward declarations
struct HWND__;
typedef HWND__* HWND;

struct ID3D11ShaderResourceView;

// Callback function types
typedef std::function<void(const std::string&, const std::string&)> LoginCallback;
typedef std::function<void(const std::string&, const std::string&, const std::string&)> RegisterCallback;
typedef std::function<void(const std::string&, const std::string&)> LicenseCallback;
typedef std::function<void()> ExitCallback;
typedef std::function<void(const std::string&)> FilestreamCallback;
typedef std::function<void(bool)> AuthModeCallback;

struct user_subscription {
    std::string plan;
    std::string expires_at;
    std::string status;
    std::string plan_id;
    std::string default_file_id;
    std::string product_image_mime;
    std::string product_image_updated_at;
    std::vector<unsigned char> product_image;
    std::string product_video_mime;
    std::string product_video_updated_at;
    std::vector<unsigned char> product_video;
    std::string product_video_path;
};

struct user_profile {
    std::string username;
    std::string email;
    std::string ip;
    std::vector<user_subscription*> subscriptions;
};

struct ui_config {
    const char* title = "Bootstrapper";
    const char* application_name = "TestClient";
};

struct ui_state {
    bool show_login_window = true;
    bool show_register_window = false;
    bool show_main_window = false;
    bool authenticated = false;
    bool license_only_mode = false;
    std::string status_message;
    std::string error_message;
};
class c_video_player;
class LOADER_UI_API c_loader_ui {
public:
    struct product_view {
        size_t index = 0;
        user_subscription* subscription = nullptr;
        ID3D11ShaderResourceView* image = nullptr;
        bool owns_image = false;
        bool active = false;
        std::string display_name;
        std::string display_status;
        std::string expires_at;
        c_video_player* video_player;

    };

private:

    // Callback functions
    LoginCallback login_callback;
    RegisterCallback register_callback;
    LicenseCallback license_callback;
    ExitCallback exit_callback;
    FilestreamCallback filestream_callback;
    AuthModeCallback auth_mode_callback;

    // Internal state
    bool should_close;
    bool initialized;

    std::vector<product_view> products;
    bool products_dirty = false;
    void release_product_views();
    void rebuild_product_views();
    void initialize_fallback_icons();
    void release_fallback_icons();
    std::vector<std::shared_ptr<c_video_player>> video_players;
    std::filesystem::path video_cache_directory;
    std::vector<std::filesystem::path> video_cache_paths;
    void render_login_window();
    void render_register_window();
    void render_main_window();
    void render_auth_mode_window();
    static void apply_base_theme();
public:
    c_loader_ui();
    ~c_loader_ui();

    ui_config config;
    ui_state state;
    user_profile user;

    [[nodiscard]] bool initialize(const ui_config& cfg = ui_config{});
    void shutdown();
    [[nodiscard]] bool should_run() const;
    void update();
    void render();

    // State management
    void set_authenticated(bool auth, user_profile* user);
    void set_status_message(const std::string& message);
    void set_error_message(const std::string& message);
    void set_loading(bool active);
    void show_login();
    void show_register();
    void show_main();
    void set_local_account_username(const std::string& username);
    void set_license_only_mode(bool enabled);

    // Callback setters
    void set_login_callback(LoginCallback callback);
    void set_register_callback(RegisterCallback callback);
    void set_license_callback(LicenseCallback callback);
    void set_exit_callback(ExitCallback callback);
    void set_filestream_callback(FilestreamCallback callback);
    void set_auth_mode_callback(AuthModeCallback callback);

    void handle_login_request(const std::string& username, const std::string& password);
    void handle_register_request(const std::string& username, const std::string& password, const std::string& license);
    void handle_launch_request(const std::string& file_id);
    void handle_license_redeem(const std::string& license);
    const std::vector<product_view>& get_product_views() const;

    // Utility
    void close();
};

// C-style exported functions for DLL interface
extern "C" {
    LOADER_UI_API c_loader_ui* create_loader_ui();
    LOADER_UI_API void destroy_loader_ui(c_loader_ui* ui);
    LOADER_UI_API bool ui_initialize(c_loader_ui* ui, const char* title);
    LOADER_UI_API void ui_shutdown(c_loader_ui* ui);
    LOADER_UI_API bool ui_should_run(c_loader_ui* ui);
    LOADER_UI_API void ui_update(c_loader_ui* ui);
    LOADER_UI_API void ui_render(c_loader_ui* ui);
    LOADER_UI_API void ui_set_authenticated(c_loader_ui* ui, bool auth, user_profile* new_profile);
    LOADER_UI_API void ui_set_status_message(c_loader_ui* ui, const char* message);
    LOADER_UI_API void ui_set_error_message(c_loader_ui* ui, const char* message);
    LOADER_UI_API void ui_set_loading(c_loader_ui* ui, bool loading);
    LOADER_UI_API void ui_close(c_loader_ui* ui);
    LOADER_UI_API void ui_set_local_account(c_loader_ui* ui, const char* username);
    LOADER_UI_API void ui_set_license_only_mode(c_loader_ui* ui, bool enabled);
    LOADER_UI_API void ui_set_auth_mode_callback(c_loader_ui* ui, void(*callback)(bool));

    // C-style callback setters to avoid std::function export issues
    LOADER_UI_API void ui_set_login_callback(c_loader_ui* ui, void(*callback)(const char*, const char*));
    LOADER_UI_API void ui_set_register_callback(c_loader_ui* ui, void(*callback)(const char*, const char*, const char*));
    LOADER_UI_API void ui_set_license_callback(c_loader_ui* ui, void(*callback)(const char*, const char*));
    LOADER_UI_API void ui_set_exit_callback(c_loader_ui* ui, void(*callback)());
    LOADER_UI_API void ui_set_filestream_callback(c_loader_ui* ui, void(*callback)(const char*));
}

#endif // LOADER_UI_HPP
