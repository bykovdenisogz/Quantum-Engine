#pragma once
// ============================================================================
// Quantum Engine - Browser Core
// ============================================================================

#include <string>
#include <vector>
#include <functional>

typedef void* webview_t;

namespace quantum {

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

    std::string current_url_;
    std::string home_url_;
    std::vector<std::string> history_;
    int history_pos_;
};

} // namespace quantum
