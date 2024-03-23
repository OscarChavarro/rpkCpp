#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include "skin/RadianceMethod.h"

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

// Entities
#define MGF_ENTITY_COLOR 1 // c
#define MGF_ENTITY_CCT 2 // cct
#define MGF_ENTITY_CONE 3 // cone
#define MGF_ENTITY_C_MIX 4 // cMix
#define MGF_ENTITY_C_SPEC 5 // cSpec
#define MGF_ENTITY_CXY 6 // cxy
#define MGF_ENTITY_CYLINDER 7 // cyl
#define MGF_ENTITY_ED 8 // ed
#define MGF_ENTITY_FACE 9 // f
#define MG_E_INCLUDE 10 // i
#define MG_E_IES 11 // ies
#define MGF_ENTITY_IR 12 // ir
#define MGF_ENTITY_MATERIAL 13 // m
#define MGF_ENTITY_NORMAL 14 // n
#define MGF_ENTITY_OBJECT 15 // o
#define MGF_ENTITY_POINT 16 // p
#define MGF_ENTITY_PRISM 17 // prism
#define MGF_ENTITY_RD 18 // rd
#define MGF_ENTITY_RING 19 // ring
#define MGF_ENTITY_RS 20 // rs
#define MGF_ENTITY_SIDES 21 // sides
#define MGF_ENTITY_SPHERE 22 // sph
#define MGF_ENTITY_TD 23 // td
#define MGF_ENTITY_TORUS 24 // torus
#define MGF_ENTITY_TS 25 // ts
#define MGF_ENTITY_VERTEX 26 // v
#define MGF_ENTITY_XF 27 // xf
#define MGF_ENTITY_FACE_WITH_HOLES 28 // fh (version 2 MGF)
#define MGF_TOTAL_NUMBER_OF_ENTITIES 29

#define xf_ac(xf) ((xf)==nullptr ? 0 : (xf)->xac)
#define xf_av(xf) (GLOBAL_mgf_xfLastTransform - (xf)->xac)
#define xf_argc xf_ac(GLOBAL_mgf_xfContext)
#define xf_xid(xf) ((xf)==nullptr ? 0 : (xf)->xid)

#define MGF_MAXIMUM_ENTITY_NAME_LENGTH    6

extern char GLOBAL_mgf_entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];

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

extern char *GLOBAL_mgf_errors[MGF_NUMBER_OF_ERRORS];
extern MgfReaderContext *GLOBAL_mgf_file;
extern int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, RadianceMethod *context);
extern int (*GLOBAL_mgf_support[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, RadianceMethod * /*context*/);

extern void doError(const char *errmsg);
extern void doWarning(const char *errmsg);

extern void mgfGetFilePosition(MgdReaderFilePosition *pos);
extern int mgfGoToFilePosition(MgdReaderFilePosition *pos);
extern int mgfEntity(char *name);
extern int mgfHandle(int en, int ac, char **av, RadianceMethod * /*context*/);

#endif
