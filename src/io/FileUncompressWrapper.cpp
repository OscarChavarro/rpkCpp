#include <cstring>
#include <cstdlib>

#include "common/error.h"
#include "io/FileUncompressWrapper.h"

/**
Opens a file with given name and fopen() open_mode ("w" or "r" e.g.). Returns the
FILE * or nullptr if opening the file was not succesful. Returns in isPipe whether
or not the file has been opened through a pipe. File extensions
.Z, .gz, .bz and .bz2 are recognised and lead to piped input/output with the
proper compress/uncompress commands. Also if the first character of the file name is
equal to '|', the file name is opened as a pipe.
*/
FILE *
openFileCompressWrapper(const char *fileName, const char *open_mode, int *isPipe) {
    FILE *fp = nullptr;

    if ( *open_mode != 'r' && *open_mode != 'w' && *open_mode != 'a' ) {
        logError("openFileCompressWrapper", "Invalid fopen() mode '%s'\n", open_mode);
        return fp;
    }

    if ( fileName[0] != '\0' && fileName[strlen(fileName) - 1] != '/' ) {
        int n = (int)strlen(fileName) + 20;
        char *command = new char[n];
        const char *ext = strrchr(fileName, '.');
        if ( fileName[0] == '|' ) {
            snprintf(command, n, "%s", fileName + 1);
            fp = popen(command, open_mode);
            *isPipe = true;
        } else if ( ext && strcmp(ext, ".gz") == 0 ) {
            if ( *open_mode == 'r' ) {
                snprintf(command, n, "gunzip < %s", fileName);
            } else {
                snprintf(command, n, "gzip > %s", fileName);
            }
            fp = popen(command, open_mode);
            *isPipe = true;
        } else if ( ext && strcmp(ext, ".Z") == 0 ) {
            if ( *open_mode == 'r' ) {
                snprintf(command, n, "uncompress < %s", fileName);
            } else {
                snprintf(command, n, "compress > %s", fileName);
            }
            fp = popen(command, open_mode);
            *isPipe = true;
        } else if ( ext && strcmp(ext, ".bz") == 0 ) {
            if ( *open_mode == 'r' ) {
                snprintf(command, n, "bunzip < %s", fileName);
            } else {
                snprintf(command, n, "bzip > %s", fileName);
            }
            fp = popen(command, open_mode);
            *isPipe = true;
        } else if ( ext && strcmp(ext, ".bz2") == 0 ) {
            if ( *open_mode == 'r' ) {
                snprintf(command, n, "bunzip2 < %s", fileName);
            } else {
                snprintf(command, n, "bzip2 > %s", fileName);
            }
            fp = popen(command, open_mode);
            *isPipe = true;
        } else {
            fp = fopen(fileName, open_mode);
            *isPipe = false;
        }

        delete[] command;

        if ( !fp ) {
            logError(nullptr, "Can't open file '%s' for %s",
                     fileName, *open_mode == 'r' ? "reading" : "writing");
        }
        return fp;
    }

    return nullptr;
}

/**
Closes the file taking into account whether or not it is a pipe
*/
void
closeFile(FILE *fp, int isPipe) {
    if ( fp ) {
        if ( isPipe ) {
            pclose(fp);
        } else {
            fclose(fp);
        }
    }
}
