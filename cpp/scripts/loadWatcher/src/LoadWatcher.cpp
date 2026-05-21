#include "LoadWatcher.h"

#include <stdio.h>
#include <unistd.h>

#include <map>
#include <set>

#include "ProcReader.h"
#include "ProcessSnapshot.h"

namespace {

const char *TARGET_PROCESS_NAME = "rpk";
const unsigned int POLL_INTERVAL_SECONDS = 1;

void printProcessReport(const ProcessStats &stats) {
    const char *inputFile = stats.inputFile.empty() ? "<unknown>" : stats.inputFile.c_str();
    printf(
        "Process rpk file=%s pid=%d rss_min_bytes=%llu rss_max_bytes=%llu vmem_min_bytes=%llu vmem_max_bytes=%llu user_time_min_seconds=%.6f user_time_max_seconds=%.6f\n",
        inputFile,
        stats.pid,
        stats.minRssBytes,
        stats.maxRssBytes,
        stats.minVirtualBytes,
        stats.maxVirtualBytes,
        stats.minUserSeconds,
        stats.maxUserSeconds);
    fflush(stdout);
}

}

int
LoadWatcher::run() const {
    bool batchStarted = false;
    std::map<pid_t, ProcessStats> trackedProcesses;

    while ( true ) {
        java::ArrayList<ProcessSnapshot> snapshots = ProcReader::readProcessesByName(TARGET_PROCESS_NAME);
        std::set<pid_t> livePids;

        for ( long int i = 0; i < snapshots.size(); i++ ) {
            const ProcessSnapshot &snapshot = snapshots[i];
            livePids.insert(snapshot.pid);

            std::map<pid_t, ProcessStats>::iterator found = trackedProcesses.find(snapshot.pid);
            if ( found == trackedProcesses.end() ) {
                trackedProcesses.insert(std::make_pair(snapshot.pid, ProcessStats(snapshot)));
            } else {
                found->second.update(snapshot);
            }
        }

        if ( snapshots.size() > 0 ) {
            batchStarted = true;
        }

        for ( std::map<pid_t, ProcessStats>::iterator it = trackedProcesses.begin(); it != trackedProcesses.end(); ) {
            if ( livePids.find(it->first) == livePids.end() ) {
                printProcessReport(it->second);
                std::map<pid_t, ProcessStats>::iterator eraseIt = it;
                ++it;
                trackedProcesses.erase(eraseIt);
            } else {
                ++it;
            }
        }

        if ( batchStarted && trackedProcesses.empty() && snapshots.size() == 0 ) {
            break;
        }

        sleep(POLL_INTERVAL_SECONDS);
    }

    return 0;
}
