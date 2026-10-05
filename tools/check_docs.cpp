// Maintained Markdown navigation guard. Supplied spec parts are source material:
// their example links are not repository navigation and are deliberately excluded.
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
std::string read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer; buffer << file.rdbuf(); return buffer.str();
}
std::string prose(const std::string& text) {
    std::istringstream input(text); std::string line, result; char fence = 0;
    while (std::getline(input, line)) {
        const auto first = line.find_first_not_of(" \t\r");
        const auto start = first == std::string::npos ? std::string{} : line.substr(first);
        if (start.starts_with("```") || start.starts_with("~~~")) {
            if (!fence) fence = start[0]; else if (fence == start[0]) fence = 0;
            result += '\n'; continue;
        }
        result += fence ? "\n" : line + '\n';
    }
    return result;
}
std::string decode(const std::string& value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size() &&
            std::isxdigit(static_cast<unsigned char>(value[i+1])) &&
            std::isxdigit(static_cast<unsigned char>(value[i+2]))) {
            out += static_cast<char>(std::stoi(value.substr(i+1,2), nullptr, 16)); i += 2;
        } else out += value[i];
    }
    return out;
}
std::string slug(std::string heading) {
    heading = std::regex_replace(heading, std::regex(R"(!?\[([^\]]+)\]\([^)]*\))"), "$1");
    heading = std::regex_replace(heading, std::regex("<[^>]*>"), "");
    std::string result;
    for (std::size_t i=0; i<heading.size(); ++i) {
        const auto c=static_cast<unsigned char>(heading[i]);
        if (c<128) {
            if (std::isalnum(c) || c=='_' || c=='-') result += static_cast<char>(std::tolower(c));
            else if (c==' ') result += '-';
        } else {
            const auto length=(c&0xe0)==0xc0 ? 2u : (c&0xf0)==0xe0 ? 3u : 4u;
            const auto token=heading.substr(i,length);
            // Markdown headings here use UTF-8 en/em dashes and typographic punctuation.
            // Keep non-ASCII words; GitHub removes these punctuation/arrow/emoji blocks.
            const auto next=i+1<heading.size() ? static_cast<unsigned char>(heading[i+1]) : 0u;
            if (!(c==0xe2 && (next==0x80 || next==0x81 || next==0x86 || next==0x87)) && c!=0xf0)
                result += token;
            i += length-1;
        }
    }
    return result;
}
std::set<std::string> anchors(const std::string& text) {
    std::set<std::string> result; std::map<std::string, unsigned> seen;
    const auto body=prose(text);
    const std::regex heading(R"(^ {0,3}#{1,6}\s+(.+?)\s*#*\s*$)");
    std::istringstream input(body); std::string line; std::smatch match;
    while (std::getline(input,line)) if (std::regex_match(line,match,heading)) {
        const auto name=slug(match[1]); const auto count=seen[name]++;
        result.insert(name+(count ? "-"+std::to_string(count) : ""));
    }
    const std::regex explicitAnchor(R"anchor(<a\s+(?:[^>]*\s)?(?:id|name)=["']([^"']+))anchor");
    for (std::sregex_iterator it(body.begin(),body.end(),explicitAnchor), end; it!=end; ++it)
        result.insert((*it)[1]);
    return result;
}
std::vector<std::string> links(const std::string& text) {
    const auto body=prose(text); std::vector<std::string> result;
    const std::regex link(R"(\]\((<?[^\s)]+>?))");
    for (std::sregex_iterator it(body.begin(),body.end(),link), end; it!=end; ++it) {
        auto url=(*it)[1].str();
        if (url.starts_with('<') && url.ends_with('>')) url=url.substr(1,url.size()-2);
        result.push_back(url);
    }
    return result;
}
bool external(const std::string& url) {
    return url.starts_with("//") || std::regex_search(url,std::regex(R"(^[A-Za-z][A-Za-z0-9+.-]*:)"));
}
bool sourcePart(const fs::path& path) {
    const auto name=path.generic_string();
    return name.starts_with("docs/specs/") && path.filename().string().starts_with("part-");
}
// In-memory existence uses the same relative resolution as a checkout, for negative fixtures.
using Documents = std::map<std::string, std::string>;
std::string normalized(const fs::path& path) {
    auto name=path.lexically_normal().generic_string();
    while (name.size()>1 && name.ends_with('/')) name.pop_back();
    return name;
}
std::vector<std::string> check(const Documents& docs, const std::set<std::string>& files) {
    std::vector<std::string> errors;
    for (const auto& [path,text]:docs) {
        if (sourcePart(path)) continue;
        for (auto url:links(text)) {
            if (external(url)) continue;
            url=decode(url); const auto hash=url.find('#');
            const auto target=url.substr(0,hash);
            const auto resolved=target.empty() ? path :
                normalized(fs::path(path).parent_path()/target);
            if (!files.contains(resolved)) { errors.push_back(path+": missing "+url); continue; }
            if (hash!=std::string::npos && hash+1<url.size() && docs.contains(resolved) &&
                !anchors(docs.at(resolved)).contains(url.substr(hash+1)))
                errors.push_back(path+": missing anchor "+url);
        }
    }
    // Each maintained document has one folder index. Archive/evidence/specs have their own conventions.
    for (const auto& folder:{"docs","docs/reference","docs/plans","docs/audits","docs/decisions"}) {
        const auto index=std::string(folder)+"/README.md";
        if (!docs.contains(index)) { errors.push_back("Missing index: "+index); continue; }
        std::set<std::string> listed;
        for (const auto& url:links(docs.at(index))) {
            if (!external(url)) listed.insert(normalized(fs::path(folder)/decode(url.substr(0,url.find('#')))));
        }
        for (const auto& [path,text]:docs) {
            (void)text;
            if (fs::path(path).parent_path().generic_string()==folder && path!=index && !listed.contains(path))
                errors.push_back(index+": unindexed "+path);
        }
    }
    return errors;
}
int selfTest() {
    Documents docs{{"README.md","[Guide](docs/reference/Guide.md#same-1)\n[Decision](docs/decisions/RECORD.md#d1)\n"},
        {"docs/README.md","# Docs\n"},{"docs/reference/README.md","[Guide](Guide.md)\n"},
        {"docs/reference/Guide.md","# Same\n# Same\n```md\n[example](missing.md)\n```\n"},
        {"docs/plans/README.md","# Plans\n"},{"docs/audits/README.md","# Audits\n"},
        {"docs/decisions/README.md","[Record](RECORD.md)\n"},
        {"docs/decisions/RECORD.md","# Decisions\n<a id=\"d1\"></a>\n"},
        {"docs/specs/link/part-01.md","[source example](missing.md)\n"}};
    std::set<std::string> files; for (const auto& [path,text]:docs) { (void)text; files.insert(path); }
    if (!check(docs,files).empty()) return 1;
    docs["README.md"]+="[Broken](missing.md)\n";
    if (check(docs,files).empty()) return 1;
    docs["README.md"]="[Stale](docs/reference/Guide.md#absent)\n";
    if (check(docs,files).empty()) return 1;
    docs["README.md"]="[Encoded](docs/reference/Guide.md#same%2D1)\n";
    if (!check(docs,files).empty()) return 1;
    docs["docs/reference/README.md"]="# Unindexed\n";
    if (check(docs,files).empty()) return 1;
    std::cout << "Documentation positive/negative fixtures passed\n"; return 0;
}
}
int main(int argc,char** argv) {
    if (argc==2 && std::string(argv[1])=="--self-test") return selfTest();
    if (argc!=2) { std::cerr << "Usage: trafficsim-check-docs <repository> | --self-test\n"; return 2; }
    const fs::path root=argv[1]; Documents docs; std::set<std::string> files;
    if (!fs::is_directory(root)) { std::cerr << "Not a repository directory: " << root << '\n'; return 2; }
    for (auto it=fs::recursive_directory_iterator(root); it!=fs::recursive_directory_iterator(); ++it) {
        const auto name=it->path().filename().string();
        if (it->is_directory()) {
            if (name.starts_with('.') || name.starts_with("build") || name=="node_modules" || name=="dist" || name=="out")
                it.disable_recursion_pending();
        }
        const auto path=it->path().lexically_relative(root).generic_string(); files.insert(path);
        if (it->is_regular_file() && it->path().extension()==".md") docs.emplace(path,read(it->path()));
    }
    const auto errors=check(docs,files);
    for (const auto& error:errors) std::cerr << error << '\n';
    if (errors.empty()) std::cout << "Documentation links, anchors and maintained indexes passed\n";
    return errors.empty() ? 0 : 1;
}
