// ============================================================================
// Quantum Engine - Browser Core Implementation
// ============================================================================
// Uses the webview C API (webview_t handle) for compatibility.
// ============================================================================

#include "browser.h"
#include <webview/webview.h>
#include <sstream>
#include <cstring>

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
{
    g_browser = this;
}

Browser::~Browser() {
    if (wv_) {
        webview_destroy(wv_);
        wv_ = nullptr;
    }
    g_browser = nullptr;
}

// ============================================================================
// URL formatting
// ============================================================================

std::string Browser::format_url(const std::string& input) const {
    if (input.empty()) return home_url_;
    if (input.find("://") != std::string::npos) return input;
    if (input.size() > 2 && input.substr(0, 2) == "//") return "https:" + input;
    if (input.find('.') != std::string::npos && input.find(' ') == std::string::npos)
        return "https://" + input;
    return "https://www.google.com/search?q=" + input;
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

// ============================================================================
// Load the browser chrome UI
// ============================================================================

void Browser::load_chrome() {
    static const char* HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#0f0f17;color:#e0e0e0;height:100vh;display:flex;flex-direction:column;overflow:hidden}
.tabs{display:flex;background:#0a0a12;padding:6px 8px 0;gap:3px;align-items:flex-end}
.tab{background:#1a1a2e;color:#888;padding:8px 16px;border-radius:8px 8px 0 0;cursor:pointer;font-size:12px;max-width:180px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;display:flex;align-items:center;gap:8px;transition:all .15s}
.tab:hover{background:#252540}.tab.active{background:#1e1e36;color:#fff}
.tab .x{opacity:.5;font-size:14px;border-radius:3px;padding:0 3px}.tab .x:hover{background:#e74c3c;color:#fff;opacity:1}
.add-tab{background:none;border:none;color:#555;font-size:20px;cursor:pointer;padding:6px 12px;border-radius:6px}.add-tab:hover{background:#1a1a2e;color:#aaa}
.toolbar{display:flex;background:#12121e;padding:8px 10px;gap:8px;align-items:center;border-bottom:1px solid #222}
.btn{background:#1a1a2e;border:none;color:#ccc;width:32px;height:32px;border-radius:8px;cursor:pointer;font-size:16px;display:flex;align-items:center;justify-content:center;transition:all .15s}
.btn:hover{background:#2a2a44;color:#fff}.btn:active{background:#3a3a54}
.url-box{flex:1;background:#1a1a2e;border:1px solid #333;border-radius:8px;color:#fff;padding:0 14px;height:32px;font-size:13px;outline:none;transition:border .2s}
.url-box:focus{border-color:#6c5ce7}.url-box::placeholder{color:#555}
.viewport{flex:1;background:#0f0f17;display:flex;align-items:center;justify-content:center;color:#444;flex-direction:column;gap:12px}
.viewport h2{font-weight:400;font-size:24px;color:#555}
.status{display:flex;background:#0a0a12;padding:4px 12px;font-size:11px;color:#555;border-top:1px solid #1a1a2e;justify-content:space-between}
</style>
</head>
<body>
<div class="tabs" id="tabs"><button class="add-tab" onclick="Q.newTab()">+</button></div>
<div class="toolbar">
<button class="btn" onclick="Q.back()" id="bB">&#9664;</button>
<button class="btn" onclick="Q.fwd()" id="bF">&#9654;</button>
<button class="btn" onclick="Q.reload()">&#8635;</button>
<button class="btn" onclick="Q.home()">&#8962;</button>
<input class="url-box" id="url" placeholder="Enter URL or search..." onkeydown="if(event.key==='Enter')Q.nav(this.value)">
</div>
<div class="viewport"><h2>Quantum Engine</h2><p>Enter a URL above to begin browsing</p></div>
<div class="status"><span id="st">Ready</span></div>
<script>
var Q={
    tabs:[],aid:0,nid:1,hist:[],hpos:-1,
    init:function(){this.newTab()},
    newTab:function(url){
        var id=this.nid++;
        this.tabs.push({id:id,title:'New Tab',url:url||''});
        this.aid=id;
        this.hist=[];this.hpos=-1;
        this.renderTabs();
        if(url){this.nav(url)}
        document.getElementById('url').focus();
    },
    closeTab:function(id,e){
        e&&e.stopPropagation();
        if(this.tabs.length<=1){this.newTab();return}
        var i=this.tabs.findIndex(t=>t.id===id);
        this.tabs.splice(i,1);
        if(this.aid===id){this.aid=this.tabs[Math.min(i,this.tabs.length-1)].id}
        this.renderTabs();
    },
    switchTab:function(id){
        this.aid=id;
        var t=this.tabs.find(t=>t.id===id);
        this.renderTabs();
        document.getElementById('url').value=t?t.url:'';
    },
    nav:function(url){
        if(!url||!url.trim())return;
        url=url.trim();
        var u=Q.formatUrl(url);
        var t=this.tabs.find(t=>t.id===this.aid);
        if(t){t.url=u;t.title=u;this.renderTabs()}
        this.hist.push(u);this.hpos=this.hist.length-1;
        document.getElementById('st').textContent='Loading: '+u;
        document.getElementById('url').value=u;
        qengine_navigate(u);
    },
    back:function(){qengine_go_back()},
    fwd:function(){qengine_go_forward()},
    reload:function(){qengine_reload()},
    home:function(){this.nav('https://www.google.com')},
    renderTabs:function(){
        var c=document.getElementById('tabs');
        var self=this;
        c.innerHTML='';
        this.tabs.forEach(function(t){
            var d=document.createElement('div');
            d.className='tab'+(t.id===self.aid?' active':'');
            var title=t.title||'New Tab';
            if(title.length>25)title=title.substring(0,22)+'...';
            d.innerHTML='<span>'+title+'</span>'+(self.tabs.length>1?'<span class="x" onclick="Q.closeTab('+t.id+',event)">&times;</span>':'');
            d.onclick=function(){self.switchTab(t.id)};
            c.appendChild(d);
        });
        var btn=document.createElement('button');
        btn.className='add-tab';btn.textContent='+';btn.onclick=function(){self.newTab()};
        c.appendChild(btn);
    },
    formatUrl:function(s){
        if(!s)return 'https://www.google.com';
        if(s.indexOf('://')>=0)return s;
        if(s.substring(0,2)=='//')return 'https:'+s;
        if(s.indexOf('.')>=0&&s.indexOf(' ')<0)return 'https://'+s;
        return 'https://www.google.com/search?q='+encodeURIComponent(s);
    }
};
Q.init();
</script>
</body>
</html>
)rawliteral";

    webview_set_html(wv_, HTML);
}

// ============================================================================
// Navigate to a web page with floating toolbar
// ============================================================================

void Browser::navigate_to(const std::string& url) {
    current_url_ = url;

    // Navigate to the target URL
    webview_navigate(wv_, url.c_str());

    // Inject floating toolbar after page loads
    webview_dispatch(wv_, [](webview_t, void* arg) {
        auto* self = static_cast<Browser*>(arg);
        static const char* TOOLBAR_JS = R"rawliteral(
(function(){
    function inject(){
        if(document.getElementById('qe-bar'))return;
        try{
            var b=document.createElement('div');
            b.id='qe-bar';
            b.style.cssText='position:fixed;top:0;left:0;right:0;z-index:2147483647;background:rgba(15,15,23,.95);backdrop-filter:blur(12px);padding:6px 12px;display:flex;align-items:center;gap:8px;border-bottom:1px solid rgba(255,255,255,.1);font-family:system-ui,sans-serif';
            var bs='background:rgba(255,255,255,.08);border:none;color:#ddd;width:30px;height:30px;border-radius:6px;cursor:pointer;font-size:15px;display:flex;align-items:center;justify-content:center';
            b.innerHTML='<button style="'+bs+'" onclick="window.qe_back()">&#9664;</button>'
                +'<button style="'+bs+'" onclick="window.qe_fwd()">&#9654;</button>'
                +'<button style="'+bs+'" onclick="location.reload()">&#8635;</button>'
                +'<button style="'+bs+'" onclick="window.qe_home()">&#8962;</button>'
                +'<span style="color:#6c5ce7;font-size:11px;font-weight:600">Quantum Engine</span>';
            document.body.prepend(b);
            document.body.style.paddingTop='44px';
        }catch(e){}
    }
    var tries=0;
        (function poll(){if(document.body)inject();else if(tries++<30)setTimeout(poll,200)})();
})();
)rawliteral";
        webview_eval(self->wv_, TOOLBAR_JS);
    }, this);
}

// ============================================================================
// Helper to bind a named JS function
// ============================================================================

void Browser::bind_js(const std::string& name,
                      std::function<std::string(const std::string&)> fn) {
    size_t idx = g_bindings.size();
    g_bindings.push_back({std::move(fn)});
    webview_bind(wv_, name.c_str(), dispatch_bind,
                 reinterpret_cast<void*>(idx));
}

// ============================================================================
// Run
// ============================================================================

void Browser::run() {
    wv_ = webview_create(0, nullptr);
    if (!wv_) return;

    webview_set_title(wv_, "Quantum Engine");
    webview_set_size(wv_, 1280, 800, WEBVIEW_HINT_NONE);

    // Bind JS -> C++ functions
    bind_js("qengine_navigate", [this](const std::string& json) -> std::string {
        std::string url = json;
        if (!url.empty() && url.front() == '[') url = url.substr(1);
        if (!url.empty() && url.back() == ']') url.pop_back();
        if (!url.empty() && url.front() == '"') url = url.substr(1);
        if (!url.empty() && url.back() == '"') url.pop_back();
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

    // Also bind for the floating toolbar
    bind_js("qe_back", [this](const std::string&) -> std::string {
        webview_eval(wv_, "history.back()");
        return "ok";
    });

    bind_js("qe_fwd", [this](const std::string&) -> std::string {
        webview_eval(wv_, "history.forward()");
        return "ok";
    });

    bind_js("qe_home", [this](const std::string&) -> std::string {
        load_chrome();
        return "ok";
    });

    // Load the chrome UI
    load_chrome();

    // Run event loop
    webview_run(wv_);

    webview_destroy(wv_);
    wv_ = nullptr;
}

} // namespace quantum
