#include "ProcReader.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace {

bool isNumericName(const char *name) {
    if ( name == nullptr || *name == '\0' ) {
        return false;
    }

    for ( const char *cursor = name; *cursor != '\0'; cursor++ ) {
        if ( !isdigit((unsigned char)*cursor) ) {
            return false;
        }
    }

    return true;
}

bool readProcessName(pid_t pid, std::string *name) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    FILE *file = fopen(path, "r");
    if ( file == nullptr ) {
        return false;
    }

    char buffer[256];
    if ( fgets(buffer, sizeof(buffer), file) == nullptr ) {
        fclose(file);
        return false;
    }
    fclose(file);

    size_t length = strlen(buffer);
    while ( length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r') ) {
        buffer[length - 1] = '\0';
        length--;
    }

    *name = buffer;
    return true;
}

bool readCommandLine(pid_t pid, std::vector<std::string> *arguments) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    FILE *file = fopen(path, "rb");
    if ( file == nullptr ) {
        return false;
    }

    char buffer[8192];
    size_t readCount = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if ( readCount == 0 ) {
        return false;
    }

    size_t start = 0;
    for ( size_t i = 0; i < readCount; i++ ) {
        if ( buffer[i] == '\0' ) {
            if ( i > start ) {
                arguments->push_back(std::string(buffer + start, i - start));
            }
            start = i + 1;
        }
    }

    if ( start < readCount ) {
        arguments->push_back(std::string(buffer + start, readCount - start));
    }

    return !arguments->empty();
}

std::string chooseInputFile(const std::vector<std::string> &arguments) {
    for ( size_t i = 1; i < arguments.size(); i++ ) {
        if ( arguments[i].find("etc") != std::string::npos ) {
            return arguments[i];
        }
    }

    if ( arguments.size() > 1 ) {
        return arguments[1];
    }

    return std::string();
}

bool readVirtualBytes(pid_t pid, unsigned long long *virtualBytes) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *file = fopen(path, "r");
    if ( file == nullptr ) {
        return false;
    }

    char line[512];
    while ( fgets(line, sizeof(line), file) != nullptr ) {
        unsigned long long kiloBytes = 0;
        if ( sscanf(line, "VmSize: %llu kB", &kiloBytes) == 1 ) {
            fclose(file);
            *virtualBytes = kiloBytes * 1024ULL;
            return true;
        }
    }

    fclose(file);
    return false;
}

bool readResidentBytes(pid_t pid, unsigned long long *rssBytes) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *file = fopen(path, "r");
    if ( file == nullptr ) {
        return false;
    }

    char line[512];
    while ( fgets(line, sizeof(line), file) != nullptr ) {
        unsigned long long kiloBytes = 0;
        if ( sscanf(line, "VmRSS: %llu kB", &kiloBytes) == 1 ) {
            fclose(file);
            *rssBytes = kiloBytes * 1024ULL;
            return true;
        }
    }

    fclose(file);
    return false;
}

bool readUserSeconds(pid_t pid, double *userSeconds) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *file = fopen(path, "r");
    if ( file == nullptr ) {
        return false;
    }

    char buffer[4096];
    if ( fgets(buffer, sizeof(buffer), file) == nullptr ) {
        fclose(file);
        return false;
    }
    fclose(file);

    char *lastParen = strrchr(buffer, ')');
    if ( lastParen == nullptr || *(lastParen + 1) == '\0' ) {
        return false;
    }

    char *cursor = lastParen + 2;
    int fieldIndex = 3;
    char *savePtr = nullptr;
    char *token = strtok_r(cursor, " ", &savePtr);
    long ticksPerSecond = sysconf(_SC_CLK_TCK);

    while ( token != nullptr ) {
        if ( fieldIndex == 14 ) {
            unsigned long long utimeTicks = strtoull(token, nullptr, 10);
            *userSeconds = (double)utimeTicks / (double)ticksPerSecond;
            return true;
        }

        fieldIndex++;
        token = strtok_r(nullptr, " ", &savePtr);
    }

    return false;
}

bool readSnapshot(pid_t pid, ProcessSnapshot *snapshot) {
    std::vector<std::string> arguments;
    unsigned long long rssBytes = 0;
    unsigned long long virtualBytes = 0;
    double userSeconds = 0.0;

    readCommandLine(pid, &arguments);
    if ( !readResidentBytes(pid, &rssBytes) ) {
        return false;
    }
    if ( !readVirtualBytes(pid, &virtualBytes) ) {
        return false;
    }
    if ( !readUserSeconds(pid, &userSeconds) ) {
        return false;
    }

    snapshot->pid = pid;
    snapshot->inputFile = chooseInputFile(arguments);
    snapshot->rssBytes = rssBytes;
    snapshot->virtualBytes = virtualBytes;
    snapshot->userSeconds = userSeconds;
    return true;
}

}

std::vector<ProcessSnapshot>
ProcReader::readProcessesByName(const char *processName) {
    std::vector<ProcessSnapshot> snapshots;

    DIR *directory = opendir("/proc");
    if ( directory == nullptr ) {
        return snapshots;
    }

    struct dirent *entry = nullptr;
    while ( (entry = readdir(directory)) != nullptr ) {
        if ( !isNumericName(entry->d_name) ) {
            continue;
        }

        pid_t pid = (pid_t)atoi(entry->d_name);
        std::string name;
        if ( !readProcessName(pid, &name) ) {
            continue;
        }

        if ( name != processName ) {
            continue;
        }

        ProcessSnapshot snapshot;
        if ( readSnapshot(pid, &snapshot) ) {
            snapshots.push_back(snapshot);
        }
    }

    closedir(directory);
    return snapshots;
}
