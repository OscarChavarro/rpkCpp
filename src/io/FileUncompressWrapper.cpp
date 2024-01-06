#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "FileUncompressWrapper.h"
#include "common/error.h"

/**
Opens a file with given name and fopen() open_mode ("w" or "r" e.g.). Returns the
FILE * or nullptr if opening the file was not succesful. Returns in ispipe whether
or not the file has been opened through a pipe. File extensions
.Z, .gz, .bz and .bz2 are recognised and lead to piped input/output with the
proper compress/uncompress commands. Also if the first character of the file name is
equal to '|', the file name is opened as a pipe.
*/
FILE *
OpenFile(const char *filename, const char *open_mode, int *ispipe) {
    FILE *fp = (FILE *) nullptr;

    if ( (*open_mode != 'r' && *open_mode != 'w' && *open_mode != 'a') ) {
        logError("OpenFile", "Invalid fopen() mode '%s'\n",
                 open_mode ? open_mode : "(null)");
        return fp;
    }

    if ( filename[0] != '\0' && filename[strlen(filename) - 1] != '/' ) {
        char *cmd = (char *)malloc(strlen(filename) + 20);
        char *ext = (char *)strrchr(filename, '.');
        if ( filename[0] == '|' ) {
            sprintf(cmd, "%s", filename + 1);
            fp = popen(cmd, open_mode);
            *ispipe = true;
        } else if ( ext && strcmp(ext, ".gz") == 0 ) {
            if ( *open_mode == 'r' ) {
                sprintf(cmd, "gunzip < %s", filename);
            } else {
                sprintf(cmd, "gzip > %s", filename);
            }
            fp = popen(cmd, open_mode);
            *ispipe = true;
        } else if ( ext && strcmp(ext, ".Z") == 0 ) {
            if ( *open_mode == 'r' ) {
                sprintf(cmd, "uncompress < %s", filename);
            } else {
                sprintf(cmd, "compress > %s", filename);
            }
            fp = popen(cmd, open_mode);
            *ispipe = true;
        } else if ( ext && strcmp(ext, ".bz") == 0 ) {
            if ( *open_mode == 'r' ) {
                sprintf(cmd, "bunzip < %s", filename);
            } else {
                sprintf(cmd, "bzip > %s", filename);
            }
            fp = popen(cmd, open_mode);
            *ispipe = true;
        } else if ( ext && strcmp(ext, ".bz2") == 0 ) {
            if ( *open_mode == 'r' ) {
                sprintf(cmd, "bunzip2 < %s", filename);
            } else {
                sprintf(cmd, "bzip2 > %s", filename);
            }
            fp = popen(cmd, open_mode);
            *ispipe = true;
        } else {
            fp = fopen(filename, open_mode);
            *ispipe = false;
        }

        free(cmd);

        if ( !fp ) {
            logError(nullptr, "Can't open file '%s' for %s",
                     filename, *open_mode == 'r' ? "reading" : "writing");
        }
        return fp;
    }

    return (FILE *) nullptr;
}

/**
Closes the file taking into account whether or not it is a pipe
*/
void
CloseFile(FILE *fp, int ispipe) {
    if ( fp ) {
        if ( ispipe ) {
            pclose(fp);
        } else {
            fclose(fp);
        }
    }
}
