#pragma once
// ============================================================================
// Quantum Engine - Browser Core
// ============================================================================

#include <string>
#include <vector>
#include <functional>
#include <map>

typedef void* webview_t;

namespace quantum {

enum class Theme { Dark, Light, Classic, Cyberpunk };
enum class SearchEngine { Google, Yandex, DuckDuckGo };
enum class Language { Russian, English, German, French };

struct DownloadItem {
    std::string filename;
    std::string url;
    std::string filepath;
    double progress;
    bool completed;
    std::string timestamp;
};

class Browser {
public:
    Browser();
    ~Browser();

    void run();

    webview_t wv_;

private:
    std::string format_url(const std::string& input) const;
    std::string escape_js(const std::string& s) const;
    void load_chrome();
    void navigate_to(const std::string& url);
    void bind_js(const std::string& name,
                 std::function<std::string(const std::string&)> fn);
    void bind_all();

    std::string get_theme_css() const;
    std::string get_localized_string(const std::string& key) const;
    std::string get_search_url(const std::string& query) const;
    void add_download(const std::string& url, const std::string& filename);
    void clear_browser_data();
    void toggle_private_mode();

    std::string current_url_;
    std::string home_url_;
    std::vector<std::string> history_;
    int history_pos_;

    Theme current_theme_;
    SearchEngine current_engine_;
    Language current_language_;
    bool private_mode_;
    std::vector<DownloadItem> downloads_;
    std::string downloads_path_;
    std::map<std::string, std::string> localized_strings_;
};

} // namespace quantum
