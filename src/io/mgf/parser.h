#ifndef MGF_MAJOR_VERSION_NUMBER

#include <cstdio>
#include <cstring>

#include "common/mymath.h"
#include "skin/RadianceMethod.h"
#include "io/mgf/Vector3Dd.h"
#include "io/mgf/MgfColorContext.h"
#include "io/mgf/mgfHandlerMaterial.h"

// Major version number
#define MGF_MAJOR_VERSION_NUMBER 2

// Entities
#define MGF_ENTITY_COLOR 1 // c
#define MGF_ENTITY_CCT 2 // cct
#define MGF_ENTITY_CONE 3 // cone
#define MGF_ENTITY_C_MIX 4 // cmix
#define MGF_ENTITY_C_SPEC 5 // cspec
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

#define MG_NAMELIST { \
    "#", \
    "c", \
    "cct", \
    "cone", \
    "cmix", \
    "cspec", \
    "cxy", \
    "cyl", \
    "ed", \
    "f", \
    "i", \
    "ies", \
    "ir", \
    "m", \
    "n", \
    "o", \
    "p", \
    "prism", \
    "rd", \
    "ring", \
    "rs", \
    "sides", \
    "sph", \
    "td", \
    "torus", \
    "ts", \
    "v", \
    "xf", \
    "fh" \
}

#define MGF_MAXIMUM_ENTITY_NAME_LENGTH    6

extern char GLOBAL_mgf_entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];
extern int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, RadianceMethod *context);
extern int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv);
extern int mgfDefaultHandlerForUnknownEntities(int ac, char **av);
extern unsigned GLOBAL_mgf_unknownEntitiesCounter;

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

extern char *GLOBAL_mgf_errors[MGF_NUMBER_OF_ERRORS];

/**
The general process for running the parser is to fill in the GLOBAL_mgf_handleCallbacks
array with handlers for each entity you know how to handle.
Then, call mg_init to fill in the rest.  This function will report
an error and quit if you try to support an inconsistent set of entities.
For each file you want to parse, call mgfLoad with the file name.
To read from standard input, use nullptr as the file name.
For additional control over error reporting and file management,
use mgfOpen, mgfReadNextLine, mgfParseCurrentLine and mgfClose instead of mgfLoad.
To globalPass an entity of your own construction to the parser, use
the mgfHandle function rather than the GLOBAL_mgf_handleCallbacks routines directly.
(The first argument to mgfHandle is the entity #, or -1.)
To free any data structures and clear the parser, use mgfClear.
If there is an error, mgfLoad, mgfOpen, mgfParseCurrentLine, mgfHandle and
mgfGoToFilePosition will return an error from the list above.  In addition,
mgfLoad will report the error to stderr.  The mgfReadNextLine routine
returns 0 when the end of file has been reached.
*/

#define MGF_MAXIMUM_INPUT_LINE_LENGTH 4096
#define MGF_MAXIMUM_ARGUMENT_COUNT (MGF_MAXIMUM_INPUT_LINE_LENGTH / 4)
#define MGF_DEFAULT_NUMBER_OF_DIVISIONS 5

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

class MgdReaderFilePosition {
public:
    int fid; // file this position is for
    int lineno; // line number in file
    long offset; // offset from beginning
};

extern MgfReaderContext *GLOBAL_mgf_file;
extern int GLOBAL_mgf_divisionsPerQuarterCircle;

extern void mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **, RadianceMethod *));
extern int mgfOpen(MgfReaderContext *, char *);
extern int mgfReadNextLine();
extern int mgfParseCurrentLine(RadianceMethod *context);
extern void mgfGetFilePosition(MgdReaderFilePosition *pos);
extern int mgfGoToFilePosition(MgdReaderFilePosition *pos);
extern void mgfClose();
extern void mgfClear();
extern int mgfHandle(int en, int ac, char **av, RadianceMethod * /*context*/);

extern int mgfEntity(char *name);
extern int mgfEntitySphere(int ac, char **av, RadianceMethod *context);
extern int mgfEntityTorus(int ac, char **av, RadianceMethod *context);
extern int mgfEntityCylinder(int ac, char **av, RadianceMethod *context);
extern int mgfEntityRing(int ac, char **av, RadianceMethod *context);
extern int mgfEntityCone(int ac, char **av, RadianceMethod *context);
extern int mgfEntityPrism(int ac, char **av, RadianceMethod *context);
extern int mgfEntityFaceWithHoles(int ac, char **av, RadianceMethod *context);

extern int isIntWords(char *);
extern int isIntDWords(char *, char *);
extern int isFloatWords(char *);
extern int isFloatDWords(char *, char *);
extern int isNameWords(char *);
extern int handleIncludedFile(int ac, char **av, RadianceMethod *context);

/**
Definitions for context handling routines (materials, colors, vectors)
*/

#define C_CMINWL 380 // Minimum wavelength
#define C_CMAXWL 780 // Maximum wavelength
#define C_CWLI ((float)(C_CMAXWL-C_CMINWL) / (float)(NUMBER_OF_SPECTRAL_SAMPLES - 1))
#define C_CMAXV 10000 // Nominal maximum sample value
#define C_CLPWM (683.0/C_CMAXV) // Peak lumens/watt multiplier
#define C_CSSPEC 01 // Flag if spectrum is set
#define C_CDSPEC 02 // Flag if defined w/ spectrum
#define C_CSXY 04 // Flag if xy is set
#define C_CDXY 010 // Flag if defined w/ xy
#define C_CSEFF 020 // Flag if efficacy set

class MgfVertexContext {
  public:
    VECTOR3Dd p; // Point
    VECTOR3Dd n; // Normal
    long xid; // Transform id of transform last time the vertex was modified (or created)
    int clock; // Incremented each change -- resettable
    void *clientData; // Client data -- initialized to nullptr by the parser
};

#define DEFAULT_VERTEX {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 0, 1, (void *)nullptr}

extern void doError(const char *errmsg);
extern void doWarning(const char *errmsg);
extern MgfColorContext *GLOBAL_mgf_currentColor;
extern char *GLOBAL_mgf_currentMaterialName;
extern MgfVertexContext *GLOBAL_mgf_currentVertex;
extern char *GLOBAL_mgf_currentVertexName;
extern MgfVertexContext GLOBAL_mgf_vertexContext;

extern int handleColorEntity(int ac, char **av, RadianceMethod * /*context*/);
extern int handleVertexEntity(int ac, char **av, RadianceMethod * /*context*/);
extern void clearContextTables();
extern MgfVertexContext *getNamedVertex(char *name);
extern void mgfContextFixColorRepresentation(MgfColorContext *clr, int fl);

/**
Definitions for hierarchical object name handler
*/

extern int GLOBAL_mgf_objectNames; // Depth of name hierarchy
extern char **GLOBAL_mgf_objectNamesList; // Names in hierarchy

extern int handleObject2Entity(int ac, char **av);

/**
Definitions for hierarchical transformation handler
*/

// Regular transformation
class XF {
  public:
    MATRIX4Dd xfm; // Transform matrix
    double sca; // Scale factor
};

// Maximum array dimensions
#define TRANSFORM_MAXIMUM_DIMENSIONS 8

class MgfTransformArrayArgument {
  public:
    short i; // Current count
    short n; // Current maximum
    char arg[8]; // String argument value
};

class MgfTransformArray {
  public:
    MgdReaderFilePosition startingPosition; // Starting position on input
    int numberOfDimensions; // Number of array dimensions
    MgfTransformArrayArgument transformArguments[TRANSFORM_MAXIMUM_DIMENSIONS];
};

class MgfTransformSpec {
  public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    XF xf; // Cumulative transformation
    MgfTransformArray *xarr; // Transformation array pointer
    MgfTransformSpec *prev; // Previous transformation context
}; // Followed by argument buffer

extern MgfTransformSpec *GLOBAL_mgf_xfContext; // Current transform context
extern char **GLOBAL_mgf_xfLastTransform; // Last transform argument
extern MgfVertexContext GLOBAL_mgf_defaultVertexContext;

#define xf_ac(xf) ((xf)==nullptr ? 0 : (xf)->xac)
#define xf_av(xf) (GLOBAL_mgf_xfLastTransform - (xf)->xac)
#define xf_argc xf_ac(GLOBAL_mgf_xfContext)
#define xf_xid(xf) ((xf)==nullptr ? 0 : (xf)->xid)

/**
The transformation handler should do most of the work that needs
doing. Just globalPass it any xf entities, then use the associated
functions to transform and translate positions, transform vectors
(without translation), rotate vectors (without scaling) and scale
values appropriately.

The routines mgfTransformPoint, mgfTransformVector and xf_rotvect take two
3-D vectors (which may be identical), transforms the second and
puts the result into the first.
*/

extern int handleTransformationEntity(int ac, char **av, RadianceMethod * /*context*/); // Handle xf entity
extern void mgfTransformPoint(VECTOR3Dd v1, VECTOR3Dd v2); // Transform point
extern void mgfTransformVector(VECTOR3Dd v1, VECTOR3Dd v2); // Transform vector

#endif
