#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Relation {
    std::string source;
    std::string target;
};

static std::string trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        start++;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        end--;
    }

    return text.substr(start, end - start);
}

static bool hasSuffix(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) {
        return false;
    }
    return text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string normalizePath(const fs::path& path) {
    std::string value = path.generic_string();
    value = trim(value);

    const std::string prefixA = "./src/";
    const std::string prefixB = "src/";

    if (value.rfind(prefixA, 0) == 0) {
        return value.substr(prefixA.size());
    }
    if (value.rfind(prefixB, 0) == 0) {
        return value.substr(prefixB.size());
    }
    return value;
}

static bool parseIncludeTarget(const std::string& line, std::string& target) {
    const std::size_t includePos = line.find("#include");
    if (includePos == std::string::npos) {
        return false;
    }

    std::size_t index = includePos + 8;
    while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index]))) {
        index++;
    }
    if (index >= line.size()) {
        return false;
    }

    const char opener = line[index];
    if (opener != '<' && opener != '"') {
        return false;
    }

    const char closer = opener == '<' ? '>' : '"';
    const std::size_t end = line.find(closer, index + 1);
    if (end == std::string::npos || end <= index + 1) {
        return false;
    }

    target = trim(line.substr(index + 1, end - index - 1));
    return !target.empty();
}

static bool isSourceFile(const fs::path& path) {
    const std::string value = path.generic_string();
    return hasSuffix(value, ".h") || hasSuffix(value, ".cpp");
}

int main() {
    const fs::path sourceRoot = fs::path(".") / "src";
    if (!fs::exists(sourceRoot) || !fs::is_directory(sourceRoot)) {
        std::cerr << "Cannot find ./src\n";
        return 1;
    }

    std::vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(sourceRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (!isSourceFile(entry.path())) {
            continue;
        }
        files.push_back(entry.path());
    }

    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        return a.generic_string() < b.generic_string();
    });

    std::vector<std::string> nodes;
    std::vector<Relation> relations;

    for (const auto& filePath : files) {
        const fs::path relativePath = fs::relative(filePath, ".");
        const std::string nodeName = normalizePath(relativePath);
        nodes.push_back(nodeName);

        std::ifstream input(filePath);
        if (!input.is_open()) {
            continue;
        }

        std::string line;
        while (std::getline(input, line)) {
            std::string target;
            if (!parseIncludeTarget(line, target)) {
                continue;
            }
            relations.push_back(Relation{nodeName, target});
        }
    }

    std::ofstream output("cache.txt", std::ios::binary);
    if (!output.is_open()) {
        std::cerr << "Cannot write cache.txt\n";
        return 1;
    }

    for (const auto& node : nodes) {
        output << "n " << node << " good\n";
    }
    output << "\n";
    for (const auto& relation : relations) {
        output << "r " << relation.source << " " << relation.target << "\n";
    }

    return 0;
}
