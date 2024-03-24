#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include "io/mgf/MgfContext.h"

// logError codes
#define MGF_OK 0 // normal return value
#define MGF_ERROR_UNKNOWN_ENTITY 1
#define MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS 2
#define MGF_ERROR_ARGUMENT_TYPE 3
#define MGF_ERROR_ILLEGAL_ARGUMENT_VALUE 4
#define MGF_ERROR_UNDEFINED_REFERENCE 5
#define MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE 6
#define MGF_ERROR_IN_INCLUDED_FILE 7
#define MGF_ERROR_OUT_OF_MEMORY 8
#define MGF_ERROR_FILE_SEEK_ERROR 9
#define MGF_ERROR_LINE_TOO_LONG 11
#define MGF_ERROR_UNMATCHED_CONTEXT_CLOSE 12
#define MGF_NUMBER_OF_ERRORS 13

#define xf_ac(xf) ((xf)==nullptr ? 0 : (xf)->xac)
#define xf_av(xf) (GLOBAL_mgf_xfLastTransform - (xf)->xac)
#define xf_argc xf_ac(GLOBAL_mgf_xfContext)
#define xf_xid(xf) ((xf)==nullptr ? 0 : (xf)->xid)

#define MGF_MAXIMUM_ARGUMENT_COUNT (MGF_MAXIMUM_INPUT_LINE_LENGTH / 4)


class MgdReaderFilePosition {
  public:
    int fid; // File this position is for
    int lineno; // Line number in file
    long offset; // Offset from beginning
};

#define MGF_MAXIMUM_INPUT_LINE_LENGTH 4096
class MgfReaderContext {
public:
    char fileName[96];
    FILE *fp; // stream pointer
    int fileContextId;
    char inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    int lineNumber;
    char isPipe; // Flag indicating whether input comes from a pipe or a real file
    MgfReaderContext *prev; // Previous context
};

extern int mgfOpen(MgfReaderContext *, char *);
extern void mgfClose();

extern char *GLOBAL_mgf_errors[MGF_NUMBER_OF_ERRORS];
extern MgfReaderContext *GLOBAL_mgf_file;

extern int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext *context);
extern int (*GLOBAL_mgf_support[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext * /*context*/);

extern void doError(const char *errmsg);
extern void doWarning(const char *errmsg);

extern void mgfGetFilePosition(MgdReaderFilePosition *pos);
extern int mgfGoToFilePosition(MgdReaderFilePosition *pos);
extern int mgfEntity(char *name, MgfContext *context);
extern int mgfHandle(int en, int ac, char **av, MgfContext * /*context*/);

#endif
