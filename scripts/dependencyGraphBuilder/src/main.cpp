#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Relation {
    std::string source;
    std::string target;
};

static bool containsNode(const std::vector<std::string>& nodes, const std::string& node) {
    for (const auto& current : nodes) {
        if (current == node) {
            return true;
        }
    }
    return false;
}

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
        std::fprintf(stderr, "Cannot find ./src\n");
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

    std::vector<std::string> sourceNodes;
    sourceNodes.reserve(files.size());
    for (const auto& filePath : files) {
        const fs::path relativePath = fs::relative(filePath, ".");
        sourceNodes.push_back(normalizePath(relativePath));
    }

    std::vector<std::string> nodes = sourceNodes;
    std::vector<Relation> relations;

    for (std::size_t index = 0; index < files.size(); index++) {
        const fs::path& filePath = files[index];
        const std::string& nodeName = sourceNodes[index];

        const std::string fileName = filePath.string();
        FILE* input = std::fopen(fileName.c_str(), "rb");
        if (input == nullptr) {
            continue;
        }

        char lineBuffer[8192];
        while (std::fgets(lineBuffer, sizeof(lineBuffer), input) != nullptr) {
            std::string line(lineBuffer);
            std::string target;
            if (!parseIncludeTarget(line, target)) {
                continue;
            }
            relations.push_back(Relation{nodeName, target});
            if (!containsNode(sourceNodes, target) && !containsNode(nodes, target)) {
                nodes.push_back(target);
            }
        }

        std::fclose(input);
    }

    FILE* output = std::fopen("cache.txt", "wb");
    if (output == nullptr) {
        std::fprintf(stderr, "Cannot write cache.txt\n");
        return 1;
    }

    for (const auto& node : nodes) {
        std::fprintf(output, "n %s good\n", node.c_str());
    }
    for (const auto& relation : relations) {
        std::fprintf(output, "r %s %s\n", relation.source.c_str(), relation.target.c_str());
    }

    std::fclose(output);
    return 0;
}
