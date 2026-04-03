#ifndef LOAD_WATCHER_PROCESS_SNAPSHOT_H
#define LOAD_WATCHER_PROCESS_SNAPSHOT_H

#include <sys/types.h>

#include <string>

struct ProcessSnapshot {
    pid_t pid;
    std::string inputFile;
    unsigned long long rssBytes;
    unsigned long long virtualBytes;
    double userSeconds;

    ProcessSnapshot():
        pid(-1),
        inputFile(),
        rssBytes(0),
        virtualBytes(0),
        userSeconds(0.0) {
    }
};

struct ProcessStats {
    pid_t pid;
    std::string inputFile;
    unsigned long long minRssBytes;
    unsigned long long maxRssBytes;
    unsigned long long minVirtualBytes;
    unsigned long long maxVirtualBytes;
    double minUserSeconds;
    double maxUserSeconds;

    ProcessStats():
        pid(-1),
        inputFile(),
        minRssBytes(0),
        maxRssBytes(0),
        minVirtualBytes(0),
        maxVirtualBytes(0),
        minUserSeconds(0.0),
        maxUserSeconds(0.0) {
    }

    explicit ProcessStats(const ProcessSnapshot &snapshot):
        pid(snapshot.pid),
        inputFile(snapshot.inputFile),
        minRssBytes(snapshot.rssBytes),
        maxRssBytes(snapshot.rssBytes),
        minVirtualBytes(snapshot.virtualBytes),
        maxVirtualBytes(snapshot.virtualBytes),
        minUserSeconds(snapshot.userSeconds),
        maxUserSeconds(snapshot.userSeconds) {
    }

    void update(const ProcessSnapshot &snapshot) {
        if ( inputFile.empty() && !snapshot.inputFile.empty() ) {
            inputFile = snapshot.inputFile;
        }
        if ( snapshot.rssBytes < minRssBytes ) {
            minRssBytes = snapshot.rssBytes;
        }
        if ( snapshot.rssBytes > maxRssBytes ) {
            maxRssBytes = snapshot.rssBytes;
        }
        if ( snapshot.virtualBytes < minVirtualBytes ) {
            minVirtualBytes = snapshot.virtualBytes;
        }
        if ( snapshot.virtualBytes > maxVirtualBytes ) {
            maxVirtualBytes = snapshot.virtualBytes;
        }
        if ( snapshot.userSeconds < minUserSeconds ) {
            minUserSeconds = snapshot.userSeconds;
        }
        if ( snapshot.userSeconds > maxUserSeconds ) {
            maxUserSeconds = snapshot.userSeconds;
        }
    }
};

#endif
