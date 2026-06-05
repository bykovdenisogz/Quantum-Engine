// ============================================================================
// Quantum Engine - Browser Core Implementation
// ============================================================================

#include "browser.h"
#include <webview/webview.h>
#include <sstream>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>

namespace quantum {

static Browser* g_browser = nullptr;

struct BindEntry {
    std::function<std::string(const std::string&)> fn;
};

static std::vector<BindEntry> g_bindings;

static void dispatch_bind(const char* id, const char* req, void* arg) {
    size_t idx = reinterpret_cast<size_t>(arg);
    if (idx < g_bindings.size()) {
        std::string result = g_bindings[idx].fn(req ? req : "");
        webview_return(g_browser->wv_, id, 0, result.c_str());
    }
}

Browser::Browser()
    : wv_(nullptr)
    , home_url_("https://www.google.com")
    , history_pos_(-1)
    , current_theme_(Theme::Dark)
    , current_engine_(SearchEngine::Google)
    , current_language_(Language::Russian)
    , private_mode_(false)
    , downloads_path_(getenv("USERPROFILE") ? std::string(getenv("USERPROFILE")) + "\\Downloads" : "C:\\Downloads")
{
    g_browser = this;

    localized_strings_ = {
        {"settings", "\xD0\x9D\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8"},
        {"theme", "\xD0\xA1\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0 \xD0\xA2\xD0\xB5\xD0\xBC\xD1\x8B"},
        {"language", "\xD0\xA1\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0 \xD0\xAF\xD0\xB7\xD1\x8B\xD0\xBA\xD0\xB0"},
        {"clear_data", "\xD0\x9E\xD1\x87\xD0\xB8\xD1\x81\xD1\x82\xD0\xBA\xD0\xB0 \xD0\xB4\xD0\xB0\xD0\xBD\xD0\xBD\xD1\x8B\xD1\x85 \xD0\xB8 \xD0\xBA\xD0\xB5\xD1\x88\xD0\xB0"},
        {"search_engine", "\xD0\x92\xD1\x8B\xD0\xB1\xD0\xBE\xD1\x80 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xBE\xD0\xB2\xD0\xBE\xD0\xB9 \xD1\x81\xD0\xB8\xD1\x81\xD1\x82\xD0\xB5\xD0\xBC\xD1\x8B"},
        {"downloads", "\xD0\x97\xD0\xB0\xD0\xB3\xD1\x80\xD1\x83\xD0\xB7\xD0\xBA\xD0\xB8"},
        {"private_mode", "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB0\xD1\x82\xD0\xBD\xD1\x8B\xD0\xB9 \xD1\x80\xD0\xB5\xD0\xB6\xD0\xB8\xD0\xBC"},
        {"dark_theme", "\xD0\xA2\xD1\x91\xD0\xBC\xD0\xBD\xD0\xB0\xD1\x8F"},
        {"light_theme", "\xD0\xA1\xD0\xB2\xD0\xB5\xD1\x82\xD0\xBB\xD0\xB0\xD1\x8F"},
        {"classic_theme", "\xD0\x9A\xD0\xBB\xD0\xB0\xD1\x81\xD1\x81\xD0\xB8\xD1\x87\xD0\xB5\xD1\x81\xD0\xBA\xD0\xB0\xD1\x8F"},
        {"cyberpunk_theme", "\xD0\x9A\xD0\xB8\xD0\xB1\xD0\xB5\xD1\x80\xD0\xBF\xD0\xB0\xD0\xBD\xD0\xBA"},
        {"google", "Google"},
        {"yandex", "Yandex"},
        {"duckduckgo", "DuckDuckGo"},
        {"russian", "\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9"},
        {"english", "English"},
        {"german", "Deutsch"},
        {"french", "Fran\xC3\xA7\xC3\xA0is"},
        {"clear_history", "\xD0\x9E\xD1\x87\xD0\xB8\xD1\x81\xD1\x82\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x80\xD0\xB8\xD1\x8E"},
        {"clear_cache", "\xD0\x9E\xD1\x87\xD0\xB8\xD1\x81\xD1\x82\xD0\xB8\xD1\x82\xD1\x8C \xD0\xBA\xD0\xB5\xD1\x88"},
        {"clear_cookies", "\xD0\x9E\xD1\x87\xD0\xB8\xD1\x81\xD1\x82\xD0\xB8\xD1\x82\xD1\x8C cookies"},
        {"clear_all", "\xD0\x9E\xD1\x87\xD0\xB8\xD1\x81\xD1\x82\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB2\xD1\x81\xD1\x91"},
        {"no_downloads", "\xD0\x9D\xD0\xB5\xD1\x82 \xD0\xB7\xD0\xB0\xD0\xB3\xD1\x80\xD1\x83\xD0\xB7\xD0\xBE\xD0\xBA"},
        {"enable", "\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C"},
        {"disable", "\xD0\x92\xD1\x8B\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C"}
    };
}

Browser::~Browser() {
    if (wv_) {
        webview_destroy(wv_);
        wv_ = nullptr;
    }
    g_browser = nullptr;
}

std::string Browser::format_url(const std::string& input) const {
    if (input.empty()) return home_url_;
    if (input.find("://") != std::string::npos) return input;
    if (input.size() > 2 && input.substr(0, 2) == "//") return "https:" + input;
    if (input.find('.') != std::string::npos && input.find(' ') == std::string::npos)
        return "https://" + input;
    return get_search_url(input);
}

std::string Browser::escape_js(const std::string& s) const {
    std::string out;
    for (char c : s) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '\'': out += "\\'"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '`':  out += "\\`"; break;
        default:   out += c;
        }
    }
    return out;
}

std::string Browser::get_theme_css() const {
    switch (current_theme_) {
    case Theme::Light:
        return "body{background:#f5f5f5;color:#333}.tabs{background:#e8e8e8}.tab{background:#d0d0d0;color:#555}.tab:hover{background:#c0c0c0}.tab.active{background:#fff;color:#333}.toolbar{background:#e0e0e0;border-bottom:1px solid #ccc}.url-box{background:#fff;border-color:#ccc;color:#333}.url-box:focus{border-color:#4a90d9}.btn{background:#d0d0d0;color:#333}.btn:hover{background:#c0c0c0}.viewport{background:#f5f5f5}.status{background:#e8e8e8;border-top:1px solid #ccc}.settings-panel{background:#fff;color:#333}.settings-item{background:#f9f9f9;border-bottom:1px solid #eee}.settings-label{color:#333}.settings-value{color:#666}.settings-select{background:#fff;border:1px solid #ccc;color:#333}.download-item{background:#f9f9f9}";
    case Theme::Classic:
        return "body{background:#c0c0c0;color:#000}.tabs{background:#a0a0a0}.tab{background:#b0b0b0;color:#000;border:1px solid #808080}.tab:hover{background:#a0a0a0}.tab.active{background:#d4d0c8;color:#000}.toolbar{background:#d4d0c8;border-bottom:1px solid #808080}.url-box{background:#fff;border-color:#808080;color:#000}.url-box:focus{border-color:#000080}.btn{background:#d4d0c8;color:#000;border:1px outset #fff}.btn:hover{background:#c0c0c0}.viewport{background:#c0c0c0}.status{background:#d4d0c8;border-top:1px solid #808080}.settings-panel{background:#d4d0c8;color:#000}.settings-item{background:#d4d0c8;border-bottom:1px solid #808080}.settings-label{color:#000}.settings-value{color:#000080}.settings-select{background:#fff;border:1px solid #808080;color:#000}.download-item{background:#d4d0c8}";
    case Theme::Cyberpunk:
        return "body{background:#0a0015;color:#ff00ff}.tabs{background:#150020}.tab{background:#200030;color:#00ffff}.tab:hover{background:#300040}.tab.active{background:#400050;color:#ff00ff}.toolbar{background:#1a0025;border-bottom:2px solid #ff00ff}.url-box{background:#200030;border-color:#ff00ff;color:#00ffff}.url-box:focus{border-color:#ffff00;box-shadow:0 0 10px #ffff00}.btn{background:#200030;color:#00ffff;border:1px solid #ff00ff}.btn:hover{background:#400050;box-shadow:0 0 10px #00ffff}.viewport{background:#0a0015}.status{background:#150020;border-top:2px solid #ff00ff}.settings-panel{background:#150020;color:#ff00ff}.settings-item{background:#200030;border-bottom:1px solid #ff00ff}.settings-label{color:#00ffff}.settings-value{color:#ffff00}.settings-select{background:#200030;border:1px solid #ff00ff;color:#00ffff}.download-item{background:#200030}";
    case Theme::Dark:
    default:
        return "body{background:#0f0f17;color:#e0e0e0}.tabs{background:#0a0a12}.tab{background:#1a1a2e;color:#888}.tab:hover{background:#252540}.tab.active{background:#1e1e36;color:#fff}.toolbar{background:#12121e;border-bottom:1px solid #222}.url-box{background:#1a1a2e;border-color:#333;color:#fff}.url-box:focus{border-color:#6c5ce7}.btn{background:#1a1a2e;color:#ccc}.btn:hover{background:#2a2a44;color:#fff}.viewport{background:#0f0f17}.status{background:#0a0a12;border-top:1px solid #1a1a2e}.settings-panel{background:#1a1a2e;color:#e0e0e0}.settings-item{background:#252540;border-bottom:1px solid #333}.settings-label{color:#e0e0e0}.settings-value{color:#888}.settings-select{background:#1a1a2e;border:1px solid #333;color:#fff}.download-item{background:#252540}";
    }
}

std::string Browser::get_localized_string(const std::string& key) const {
    auto it = localized_strings_.find(key);
    if (it != localized_strings_.end()) return it->second;
    return key;
}

std::string Browser::get_search_url(const std::string& query) const {
    std::string encoded;
    for (unsigned char c : query) {
        if (c == ' ') encoded += "+";
        else encoded += static_cast<char>(c);
    }
    switch (current_engine_) {
    case SearchEngine::Yandex:
        return "https://yandex.ru/search/?text=" + encoded;
    case SearchEngine::DuckDuckGo:
        return "https://duckduckgo.com/?q=" + encoded;
    case SearchEngine::Google:
    default:
        return "https://www.google.com/search?q=" + encoded;
    }
}

void Browser::add_download(const std::string& url, const std::string& filename) {
    DownloadItem item;
    item.url = url;
    item.filename = filename;
    item.filepath = downloads_path_ + "\\" + filename;
    item.progress = 0.0;
    item.completed = false;
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%d.%m.%Y %H:%M", std::localtime(&time));
    item.timestamp = time_str;
    downloads_.push_back(item);
}

void Browser::clear_browser_data() {
    webview_eval(wv_, R"(
        document.cookie.split(';').forEach(function(c) {
            document.cookie = c.trim().split('=')[0] + '=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/';
        });
        try { localStorage.clear(); } catch(e) {}
        try { sessionStorage.clear(); } catch(e) {}
    )");
    history_.clear();
    history_pos_ = -1;
}

void Browser::toggle_private_mode() {
    private_mode_ = !private_mode_;
}

// ============================================================================
// Parse string argument from webview callback
// ============================================================================

static std::string parse_arg(const std::string& json) {
    std::string val = json;
    if (!val.empty() && val.front() == '[') val = val.substr(1);
    if (!val.empty() && val.back() == ']') val.pop_back();
    if (!val.empty() && val.front() == '"') val = val.substr(1);
    if (!val.empty() && val.back() == '"') val.pop_back();
    return val;
}

// ============================================================================
// Bind all JS -> C++ functions
// ============================================================================

void Browser::bind_js(const std::string& name,
                      std::function<std::string(const std::string&)> fn) {
    size_t idx = g_bindings.size();
    g_bindings.push_back({std::move(fn)});
    webview_bind(wv_, name.c_str(), dispatch_bind,
                 reinterpret_cast<void*>(idx));
}

void Browser::bind_all() {
    g_bindings.clear();

    bind_js("qengine_navigate", [this](const std::string& json) -> std::string {
        std::string url = parse_arg(json);
        if (!url.empty()) navigate_to(url);
        return "ok";
    });

    bind_js("qengine_go_back", [this](const std::string&) -> std::string {
        webview_eval(wv_, "history.back()");
        return "ok";
    });

    bind_js("qengine_go_forward", [this](const std::string&) -> std::string {
        webview_eval(wv_, "history.forward()");
        return "ok";
    });

    bind_js("qengine_reload", [this](const std::string&) -> std::string {
        if (!current_url_.empty()) webview_navigate(wv_, current_url_.c_str());
        return "ok";
    });

    bind_js("qe_back", [this](const std::string&) -> std::string {
        webview_eval(wv_, "history.back()");
        return "ok";
    });

    bind_js("qe_fwd", [this](const std::string&) -> std::string {
        webview_eval(wv_, "history.forward()");
        return "ok";
    });

    bind_js("qe_home", [this](const std::string&) -> std::string {
        current_url_ = "";
        load_chrome();
        bind_all();
        return "ok";
    });

    bind_js("qengine_clear_history", [this](const std::string&) -> std::string {
        history_.clear();
        history_pos_ = -1;
        return "ok";
    });

    bind_js("qengine_clear_cache", [this](const std::string&) -> std::string {
        webview_eval(wv_, R"(
            try { localStorage.clear(); } catch(e) {}
            try { sessionStorage.clear(); } catch(e) {}
        )");
        return "ok";
    });

    bind_js("qengine_clear_cookies", [this](const std::string&) -> std::string {
        webview_eval(wv_, R"(
            document.cookie.split(';').forEach(function(c) {
                document.cookie = c.trim().split('=')[0] + '=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/';
            });
        )");
        return "ok";
    });

    bind_js("qengine_clear_all", [this](const std::string&) -> std::string {
        clear_browser_data();
        return "ok";
    });

    bind_js("qengine_set_theme", [this](const std::string& json) -> std::string {
        std::string val = parse_arg(json);
        int th = std::stoi(val.empty() ? "0" : val);
        current_theme_ = static_cast<Theme>(th);
        return "ok";
    });

    bind_js("qengine_set_language", [this](const std::string& json) -> std::string {
        std::string val = parse_arg(json);
        int lang = std::stoi(val.empty() ? "0" : val);
        current_language_ = static_cast<Language>(lang);
        return "ok";
    });

    bind_js("qengine_set_engine", [this](const std::string& json) -> std::string {
        std::string val = parse_arg(json);
        int eng = std::stoi(val.empty() ? "0" : val);
        current_engine_ = static_cast<SearchEngine>(eng);
        return "ok";
    });

    bind_js("qengine_toggle_private", [this](const std::string&) -> std::string {
        toggle_private_mode();
        return "ok";
    });

    bind_js("qengine_get_downloads", [this](const std::string&) -> std::string {
        std::string json = "[";
        for (size_t i = 0; i < downloads_.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"filename\":\"" + escape_js(downloads_[i].filename) + "\","
                    "\"url\":\"" + escape_js(downloads_[i].url) + "\","
                    "\"filepath\":\"" + escape_js(downloads_[i].filepath) + "\","
                    "\"progress\":" + std::to_string(downloads_[i].progress) + ","
                    "\"completed\":" + (downloads_[i].completed ? "true" : "false") + ","
                    "\"timestamp\":\"" + escape_js(downloads_[i].timestamp) + "\"}";
        }
        json += "]";
        return json;
    });
}

// ============================================================================
// Load chrome UI
// ============================================================================

void Browser::load_chrome() {
    std::string s_settings = get_localized_string("settings");
    std::string s_theme = get_localized_string("theme");
    std::string s_language = get_localized_string("language");
    std::string s_clear_data = get_localized_string("clear_data");
    std::string s_search_engine = get_localized_string("search_engine");
    std::string s_downloads = get_localized_string("downloads");
    std::string s_private_mode = get_localized_string("private_mode");
    std::string s_dark = get_localized_string("dark_theme");
    std::string s_light = get_localized_string("light_theme");
    std::string s_classic = get_localized_string("classic_theme");
    std::string s_cyberpunk = get_localized_string("cyberpunk_theme");
    std::string s_google = get_localized_string("google");
    std::string s_yandex = get_localized_string("yandex");
    std::string s_duckduckgo = get_localized_string("duckduckgo");
    std::string s_russian = get_localized_string("russian");
    std::string s_english = get_localized_string("english");
    std::string s_german = get_localized_string("german");
    std::string s_french = get_localized_string("french");
    std::string s_clear_history = get_localized_string("clear_history");
    std::string s_clear_cache = get_localized_string("clear_cache");
    std::string s_clear_cookies = get_localized_string("clear_cookies");
    std::string s_clear_all = get_localized_string("clear_all");
    std::string s_no_downloads = get_localized_string("no_downloads");
    std::string s_enable = get_localized_string("enable");
    std::string s_disable = get_localized_string("disable");

    int ti = static_cast<int>(current_theme_);
    int ei = static_cast<int>(current_engine_);
    int li = static_cast<int>(current_language_);
    int pi = private_mode_ ? 1 : 0;
    std::string pi_btn_text = private_mode_ ? s_disable : s_enable;

    std::string css = get_theme_css();

    std::string html;
    html += "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><style>";
    html += css;
    html += "*{margin:0;padding:0;box-sizing:border-box}";
    html += "body{font-family:'Segoe UI',system-ui,sans-serif;height:100vh;display:flex;flex-direction:column;overflow:hidden}";
    html += ".tabs{display:flex;padding:6px 8px 0;gap:3px;align-items:flex-end}";
    html += ".tab{padding:8px 16px;border-radius:8px 8px 0 0;cursor:pointer;font-size:12px;max-width:180px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;display:flex;align-items:center;gap:8px;transition:all .15s}";
    html += ".tab:hover{opacity:.8}.tab.active{opacity:1}";
    html += ".tab .x{opacity:.5;font-size:14px;border-radius:3px;padding:0 3px}.tab .x:hover{opacity:1}";
    html += ".add-tab{background:none;border:none;color:#555;font-size:20px;cursor:pointer;padding:6px 12px;border-radius:6px}";
    html += ".add-tab:hover{background:rgba(255,255,255,.1)}";
    html += ".toolbar{display:flex;padding:8px 10px;gap:8px;align-items:center}";
    html += ".btn{border:none;width:32px;height:32px;border-radius:8px;cursor:pointer;font-size:16px;display:flex;align-items:center;justify-content:center;transition:all .15s}";
    html += ".btn:hover{opacity:.8}";
    html += ".url-box{flex:1;border:1px solid #333;border-radius:8px;padding:0 14px;height:32px;font-size:13px;outline:none}";
    html += ".url-box:focus{border-color:#6c5ce7}";
    html += ".viewport{flex:1;display:flex;align-items:center;justify-content:center;color:#444;flex-direction:column;gap:12px}";
    html += ".viewport h2{font-weight:400;font-size:24px;color:#555}";
    html += ".status{display:flex;padding:4px 12px;font-size:11px;color:#555;justify-content:space-between}";
    html += ".settings-panel,.downloads-panel{display:none;flex:1;overflow-y:auto;padding:20px}";
    html += ".settings-header,.downloads-header{font-size:24px;font-weight:600;margin-bottom:20px}";
    html += ".settings-section{margin-bottom:24px}";
    html += ".settings-section-title{font-size:14px;font-weight:600;margin-bottom:12px;opacity:.7}";
    html += ".settings-item{display:flex;justify-content:space-between;align-items:center;padding:12px 16px;border-radius:8px;margin-bottom:8px}";
    html += ".settings-label{font-size:14px}";
    html += ".settings-select{padding:8px 12px;border-radius:6px;font-size:13px;outline:none}";
    html += ".settings-btn{padding:8px 16px;border-radius:6px;border:none;cursor:pointer;font-size:13px}";
    html += ".settings-btn.danger{background:#e74c3c;color:#fff}";
    html += ".private-indicator{display:none;padding:4px 12px;font-size:11px;text-align:center;animation:pulse 2s infinite}";
    html += ".private-indicator.active{display:block;background:rgba(255,0,0,.15);color:#ff6b6b}";
    html += "@keyframes pulse{0%,100%{opacity:1}50%{opacity:.7}}";
    html += ".download-item{display:flex;justify-content:space-between;align-items:center;padding:12px 16px;border-radius:8px;margin-bottom:8px}";
    html += ".download-info{flex:1}.download-name{font-size:14px;font-weight:500;margin-bottom:4px}";
    html += ".download-meta{font-size:11px;opacity:.7}";
    html += ".no-downloads{text-align:center;padding:40px;opacity:.5}";
    html += "</style></head><body>";

    // Private indicator top
    html += "<div class=\"private-indicator\" id=\"private-top\">" + s_private_mode + "</div>";

    // Tabs
    html += "<div class=\"tabs\" id=\"tabs\"></div>";

    // Toolbar
    html += "<div class=\"toolbar\">";
    html += "<button class=\"btn\" id=\"btn-back\">&#9664;</button>";
    html += "<button class=\"btn\" id=\"btn-fwd\">&#9654;</button>";
    html += "<button class=\"btn\" id=\"btn-reload\">&#8635;</button>";
    html += "<button class=\"btn\" id=\"btn-home\">&#8962;</button>";
    html += "<input class=\"url-box\" id=\"url\" placeholder=\"Enter URL or search...\">";
    html += "<button class=\"btn\" id=\"btn-settings\" title=\"Ctrl+,\">&#9881;</button>";
    html += "<button class=\"btn\" id=\"btn-downloads\" title=\"Ctrl+J\">&#8681;</button>";
    html += "<button class=\"btn\" id=\"btn-private\" title=\"Ctrl+Shift+P\">&#128274;</button>";
    html += "</div>";

    // Settings panel
    html += "<div class=\"settings-panel\" id=\"settings-panel\">";
    html += "<div class=\"settings-header\">" + s_settings + "</div>";

    html += "<div class=\"settings-section\"><div class=\"settings-section-title\">" + s_theme + "</div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_theme + "</span>";
    html += "<select class=\"settings-select\" id=\"sel-theme\">";
    html += "<option value=\"0\">" + s_dark + "</option>";
    html += "<option value=\"1\">" + s_light + "</option>";
    html += "<option value=\"2\">" + s_classic + "</option>";
    html += "<option value=\"3\">" + s_cyberpunk + "</option>";
    html += "</select></div></div>";

    html += "<div class=\"settings-section\"><div class=\"settings-section-title\">" + s_language + "</div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_language + "</span>";
    html += "<select class=\"settings-select\" id=\"sel-lang\">";
    html += "<option value=\"0\">" + s_russian + "</option>";
    html += "<option value=\"1\">" + s_english + "</option>";
    html += "<option value=\"2\">" + s_german + "</option>";
    html += "<option value=\"3\">" + s_french + "</option>";
    html += "</select></div></div>";

    html += "<div class=\"settings-section\"><div class=\"settings-section-title\">" + s_search_engine + "</div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_search_engine + "</span>";
    html += "<select class=\"settings-select\" id=\"sel-engine\">";
    html += "<option value=\"0\">" + s_google + "</option>";
    html += "<option value=\"1\">" + s_yandex + "</option>";
    html += "<option value=\"2\">" + s_duckduckgo + "</option>";
    html += "</select></div></div>";

    html += "<div class=\"settings-section\"><div class=\"settings-section-title\">" + s_clear_data + "</div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_clear_history + "</span>";
    html += "<button class=\"settings-btn danger\" id=\"btn-clear-history\">" + s_clear_history + "</button></div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_clear_cache + "</span>";
    html += "<button class=\"settings-btn danger\" id=\"btn-clear-cache\">" + s_clear_cache + "</button></div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_clear_cookies + "</span>";
    html += "<button class=\"settings-btn danger\" id=\"btn-clear-cookies\">" + s_clear_cookies + "</button></div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_clear_all + "</span>";
    html += "<button class=\"settings-btn danger\" id=\"btn-clear-all\">" + s_clear_all + "</button></div>";
    html += "</div>";

    html += "<div class=\"settings-section\"><div class=\"settings-section-title\">" + s_private_mode + "</div>";
    html += "<div class=\"settings-item\"><span class=\"settings-label\">" + s_private_mode + "</span>";
    html += "<button class=\"settings-btn\" id=\"btn-private2\">" + pi_btn_text + "</button></div>";
    html += "</div></div>";

    // Downloads panel
    html += "<div class=\"downloads-panel\" id=\"downloads-panel\">";
    html += "<div class=\"downloads-header\">" + s_downloads + "</div>";
    html += "<div id=\"downloads-list\"></div></div>";

    // Viewport
    html += "<div class=\"viewport\" id=\"viewport\"><h2>Quantum Engine</h2><p>Enter a URL above to begin browsing</p></div>";

    // Status
    html += "<div class=\"status\"><span id=\"st\">Ready</span></div>";

    // JavaScript
    html += "<script>";
    html += "var QE={";
    html += "tabs:[],aid:0,nid:1,";
    html += "theme:" + std::to_string(ti) + ",";
    html += "engine:" + std::to_string(ei) + ",";
    html += "lang:" + std::to_string(li) + ",";
    html += "privateMode:" + std::to_string(pi) + ",";
    html += "init:function(){";
    html += "  var self=this;";
    html += "  document.getElementById('btn-back').onclick=function(){qengine_go_back()};";
    html += "  document.getElementById('btn-fwd').onclick=function(){qengine_go_forward()};";
    html += "  document.getElementById('btn-reload').onclick=function(){qengine_reload()};";
    html += "  document.getElementById('btn-home').onclick=function(){qe_home()};";
    html += "  document.getElementById('btn-settings').onclick=function(){self.showSettings()};";
    html += "  document.getElementById('btn-downloads').onclick=function(){self.showDownloads()};";
    html += "  document.getElementById('btn-private').onclick=function(){self.togglePrivate()};";
    html += "  document.getElementById('btn-private2').onclick=function(){self.togglePrivate()};";
    html += "  document.getElementById('btn-clear-history').onclick=function(){qengine_clear_history()};";
    html += "  document.getElementById('btn-clear-cache').onclick=function(){qengine_clear_cache()};";
    html += "  document.getElementById('btn-clear-cookies').onclick=function(){qengine_clear_cookies()};";
    html += "  document.getElementById('btn-clear-all').onclick=function(){qengine_clear_all()};";
    html += "  document.getElementById('sel-theme').onchange=function(){self.setTheme(this.value)};";
    html += "  document.getElementById('sel-lang').onchange=function(){self.setLanguage(this.value)};";
    html += "  document.getElementById('sel-engine').onchange=function(){self.setEngine(this.value)};";
    html += "  document.getElementById('url').onkeydown=function(e){if(e.key==='Enter')self.nav(this.value)};";
    html += "  this.newTab();";
    html += "  this.updatePrivate();";
    html += "  this.loadSettings();";
    html += "},";
    html += "loadSettings:function(){";
    html += "  try{var s=localStorage.getItem('qe_settings');if(s){var d=JSON.parse(s);";
    html += "    if(d.theme!==undefined){this.theme=d.theme;document.getElementById('sel-theme').value=d.theme}";
    html += "    if(d.engine!==undefined){this.engine=d.engine;document.getElementById('sel-engine').value=d.engine}";
    html += "    if(d.lang!==undefined){this.lang=d.lang;document.getElementById('sel-lang').value=d.lang}";
    html += "  }}catch(e){}";
    html += "},";
    html += "saveSettings:function(){";
    html += "  try{localStorage.setItem('qe_settings',JSON.stringify({theme:this.theme,engine:this.engine,lang:this.lang}))}catch(e){}";
    html += "},";
    html += "newTab:function(url){";
    html += "  var id=this.nid++;";
    html += "  this.tabs.push({id:id,title:'New Tab',url:url||''});";
    html += "  this.aid=id;";
    html += "  this.renderTabs();";
    html += "  if(url)this.nav(url);";
    html += "  document.getElementById('url').focus();";
    html += "},";
    html += "closeTab:function(id,e){";
    html += "  e&&e.stopPropagation();";
    html += "  if(this.tabs.length<=1){this.newTab();return}";
    html += "  var i=this.tabs.findIndex(function(t){return t.id===id});";
    html += "  this.tabs.splice(i,1);";
    html += "  if(this.aid===id){this.aid=this.tabs[Math.min(i,this.tabs.length-1)].id}";
    html += "  this.renderTabs();";
    html += "},";
    html += "switchTab:function(id){";
    html += "  this.aid=id;";
    html += "  var t=this.tabs.find(function(t){return t.id===id});";
    html += "  this.renderTabs();";
    html += "  document.getElementById('url').value=t?t.url:'';";
    html += "},";
    html += "nav:function(url){";
    html += "  if(!url||!url.trim())return;";
    html += "  url=url.trim();";
    html += "  var u=this.formatUrl(url);";
    html += "  var t=this.tabs.find(function(t){return t.id===QE.aid});";
    html += "  if(t){t.url=u;t.title=u;this.renderTabs()}";
    html += "  document.getElementById('st').textContent='Loading: '+u;";
    html += "  document.getElementById('url').value=u;";
    html += "  qengine_navigate(u);";
    html += "},";
    html += "showSettings:function(){";
    html += "  document.getElementById('settings-panel').style.display='block';";
    html += "  document.getElementById('downloads-panel').style.display='none';";
    html += "  document.getElementById('viewport').style.display='none';";
    html += "},";
    html += "showDownloads:function(){";
    html += "  document.getElementById('downloads-panel').style.display='block';";
    html += "  document.getElementById('settings-panel').style.display='none';";
    html += "  document.getElementById('viewport').style.display='none';";
    html += "  this.updateDownloads();";
    html += "},";
    html += "setTheme:function(v){";
    html += "  this.theme=parseInt(v);";
    html += "  this.saveSettings();";
    html += "  qengine_set_theme(v);";
    html += "  qe_home();";
    html += "},";
    html += "setLanguage:function(v){";
    html += "  this.lang=parseInt(v);";
    html += "  this.saveSettings();";
    html += "  qengine_set_language(v);";
    html += "  qe_home();";
    html += "},";
    html += "setEngine:function(v){";
    html += "  this.engine=parseInt(v);";
    html += "  this.saveSettings();";
    html += "  qengine_set_engine(v);";
    html += "},";
    html += "togglePrivate:function(){";
    html += "  this.privateMode=this.privateMode?0:1;";
    html += "  this.saveSettings();";
    html += "  this.updatePrivate();";
    html += "  qengine_toggle_private();";
    html += "},";
    html += "updatePrivate:function(){";
    html += "  var el=document.getElementById('private-top');";
    html += "  if(this.privateMode){el.classList.add('active')}else{el.classList.remove('active')}";
    html += "},";
    html += "updateDownloads:function(){";
    html += "  var list=document.getElementById('downloads-list');";
    html += "  qengine_get_downloads(function(data){";
    html += "    var d=JSON.parse(data);";
    html += "    if(d.length===0){list.innerHTML='<div class=no-downloads>No downloads</div>';return}";
    html += "    list.innerHTML='';";
    html += "    d.forEach(function(item){";
    html += "      var el=document.createElement('div');el.className='download-item';";
    html += "      el.innerHTML='<div class=download-info><div class=download-name>'+item.filename+'</div><div class=download-meta>'+item.url+' | '+item.timestamp+'</div></div>';";
    html += "      list.appendChild(el);";
    html += "    });";
    html += "  });";
    html += "},";
    html += "renderTabs:function(){";
    html += "  var c=document.getElementById('tabs');var self=this;c.innerHTML='';";
    html += "  this.tabs.forEach(function(t){";
    html += "    var d=document.createElement('div');";
    html += "    d.className='tab'+(t.id===self.aid?' active':'');";
    html += "    var title=t.title||'New Tab';";
    html += "    if(title.length>25)title=title.substring(0,22)+'...';";
    html += "    var closeBtn='';if(self.tabs.length>1){closeBtn='<span class=x>&times;</span>'}";
    html += "    d.innerHTML='<span>'+title+'</span>'+closeBtn;";
    html += "    d.setAttribute('data-id',t.id);";
    html += "    d.onclick=function(e){if(e.target.className==='x'){self.closeTab(t.id,e)}else{self.switchTab(t.id)}};";
    html += "    c.appendChild(d);";
    html += "  });";
    html += "  var btn=document.createElement('button');";
    html += "  btn.className='add-tab';btn.textContent='+';";
    html += "  btn.onclick=function(){self.newTab()};";
    html += "  c.appendChild(btn);";
    html += "},";
    html += "formatUrl:function(s){";
    html += "  if(!s)return 'https://www.google.com';";
    html += "  if(s.indexOf('://')>=0)return s;";
    html += "  if(s.substring(0,2)=='//')return 'https:'+s;";
    html += "  if(s.indexOf('.')>=0&&s.indexOf(' ')<0)return 'https://'+s;";
    html += "  var engines=['https://www.google.com/search?q=','https://yandex.ru/search/?text=','https://duckduckgo.com/?q='];";
    html += "  return engines[this.engine]+encodeURIComponent(s);";
    html += "}";
    html += "};";
    html += "QE.init();";
    html += "</script></body></html>";

    webview_set_html(wv_, html.c_str());
}

// ============================================================================
// Navigate to URL
// ============================================================================

void Browser::navigate_to(const std::string& url) {
    current_url_ = url;
    webview_navigate(wv_, url.c_str());
}

// ============================================================================
// Run
// ============================================================================

void Browser::run() {
    wv_ = webview_create(0, nullptr);
    if (!wv_) return;

    webview_set_title(wv_, "Quantum Engine");
    webview_set_size(wv_, 1280, 800, WEBVIEW_HINT_NONE);

    load_chrome();
    bind_all();

    webview_run(wv_);

    webview_destroy(wv_);
    wv_ = nullptr;
}

} // namespace quantum
