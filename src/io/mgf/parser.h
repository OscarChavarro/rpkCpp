#ifndef MGF_MAJOR_VERSION_NUMBER

#include <cstdio>
#include <cstring>

#include "common/mymath.h"

// Major version number
#define MGF_MAJOR_VERSION_NUMBER 2

// Entities
#define MG_E_COLOR 1 // c
#define MG_E_CCT 2 // cct
#define MGF_ERROR_CONE 3 // cone
#define MG_E_CMIX 4 // cmix
#define MG_E_CSPEC 5 // cspec
#define MG_E_CXY 6 // cxy
#define MGF_ERROR_CYLINDER 7 // cyl
#define MG_E_ED 8 // ed
#define MG_E_FACE 9 // f
#define MG_E_INCLUDE 10 // i
#define MG_E_IES 11 // ies
#define MG_E_IR 12 // ir
#define MG_E_MATERIAL 13 // m
#define MG_E_NORMAL 14 // n
#define MG_E_OBJECT 15 // o
#define MG_E_POINT 16 // p
#define MGF_ERROR_PRISM 17 // prism
#define MG_E_RD 18 // rd
#define MGF_ERROR_RING 19 // ring
#define MG_E_RS 20 // rs
#define MG_E_SIDES 21 // sides
#define MGF_ERROR_SPHERE 22 // sph
#define MG_E_TD 23 // td
#define MGF_ERROR_TORUS 24 // torus
#define MG_E_TS 25 // ts
#define MG_E_VERTEX 26 // v
#define MG_E_XF 27 // xf
#define MG_E_FACEH 28 // fh (version 2 MGF)
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
extern int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv);
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

extern void mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **));
extern int mgfOpen(MgfReaderContext *, char *);
extern int mgfReadNextLine();
extern int mgfParseCurrentLine();
extern void mgfGetFilePosition(MgdReaderFilePosition *pos);
extern int mgfGoToFilePosition(MgdReaderFilePosition *pos);
extern void mgfClose();
extern void mgfClear();
extern int mgfHandle(int en, int ac, char **av);

extern int mgfEntity(char *name);
extern int mgfEntitySphere(int ac, char **av);
extern int mgfEntityTorus(int ac, char **av);
extern int mgfEntityCylinder(int ac, char **av);
extern int mgfEntityRing(int ac, char **av);
extern int mgfEntityCone(int ac, char **av);
extern int mgfEntityPrism(int ac, char **av);
extern int mgfEntityFaceWithHoles(int ac, char **av);

extern int isIntWords(char *);
extern int isIntDWords(char *, char *);
extern int isFloatWords(char *);
extern int isFloatDWords(char *, char *);
extern int isNameWords(char *);
extern int checkForBadArguments(int, char **, char *);
extern int handleIncludedFile(int ac, char **av);

/**
Definitions for 3-d vector manipulation functions
*/

#define FLOAT double
#define FLOAT_TINY (1e-6)
#define FLOAT_HUGE (1e10)

typedef FLOAT FVECT[3];

inline void
mgfVertexCopy(double *result, const double *source) {
    result[0] = source[0];
    result[1] = source[1];
    result[2] = source[2];
}

inline double
dotProduct(const double *a, const double *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline bool
is0Vector(double *v) {
    return dotProduct(v, v) <= FLOAT_TINY * FLOAT_TINY;
}

inline void
round0(FLOAT &x) {
    if ( x <= FLOAT_TINY && x >= -FLOAT_TINY) {
        x = 0;
    }
}

extern double normalize(FVECT);
extern void floatCrossProduct(FVECT result, const FVECT a, const FVECT b);

/**
Definitions for context handling routines (materials, colors, vectors)
*/

#define C_CMINWL 380 // Minimum wavelength
#define C_CMAXWL 780 // Maximum wavelength
#define C_CNSS 41 // Number of spectral samples
#define C_CWLI ((C_CMAXWL-C_CMINWL)/(C_CNSS-1))
#define C_CMAXV 10000 // Nominal maximum sample value
#define C_CLPWM (683.0/C_CMAXV) // Peak lumens/watt multiplier
#define C_CSSPEC 01 // Flag if spectrum is set
#define C_CDSPEC 02 // Flag if defined w/ spectrum
#define C_CSXY 04 // Flag if xy is set
#define C_CDXY 010 // Flag if defined w/ xy
#define C_CSEFF 020 // Flag if efficacy set

class MgfColorContext {
public:
    int clock; // Incremented each change
    short flags; // What's been set
    short ssamp[C_CNSS]; // Spectral samples, min wl to max
    long ssum; // Straight sum of spectral values
    float cx; // Chromaticity X value
    float cy; // chromaticity Y value
    float eff; // efficacy (lumens/watt)
};

#define C_DEFCOLOR { 1, C_CDXY|C_CSXY|C_CSSPEC|C_CSEFF,\
    {C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
    C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
    C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
    C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
    C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
    C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
    C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV},\
    (long)C_CNSS*C_CMAXV, 1.0/3.0, 1.0/3.0, 178.006 }

class MgfMaterialContext {
  public:
    int clock; // Incremented each change -- resettable
    int sided; // 1 if surface is 1-sided, 0 for 2-sided
    float nr; // Index of refraction, real and imaginary
    float ni;
    float rd; // Diffuse reflectance
    MgfColorContext rd_c; // Diffuse reflectance color
    float td; // Diffuse transmittance
    MgfColorContext td_c; // Diffuse transmittance color
    float ed; // Diffuse emittance
    MgfColorContext ed_c; // Diffuse emittance color
    float rs; // Specular reflectance
    MgfColorContext rs_c; // Specular reflectance color
    float rs_a; // Specular reflectance roughness
    float ts; // Specular transmittance
    MgfColorContext ts_c; // Specular transmittance color
    float ts_a; // Specular transmittance roughness
};

class MgfVertexContext {
  public:
    FVECT p; // Point
    FVECT n; // Normal
    long xid; // Transform id of transform last time the vertex was modified (or created)
    int clock; // Incremented each change -- resettable
    void *client_data; // Client data -- initialized to nullptr by the parser
};

#define C_DEFMATERIAL {1, 0, 1.0, 0.0, 0.0, C_DEFCOLOR, 0.0, C_DEFCOLOR, 0.0, C_DEFCOLOR,\
                    0.0, C_DEFCOLOR, 0.0, 0.0, C_DEFCOLOR, 0.0}
#define C_DEFVERTEX {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 0, 1, (void *)nullptr}

extern MgfColorContext *GLOBAL_mgf_currentColor;
extern MgfMaterialContext *GLOBAL_mgf_currentMaterial;
extern char *GLOBAL_mgf_currentMaterialName;
extern MgfVertexContext *GLOBAL_mgf_currentVertex;
extern char *GLOBAL_mgf_currentVertexName;

extern int handleColorEntity(int ac, char **av);
extern int handleMaterialEntity(int ac, char **av);
extern int handleVertexEntity(int ac, char **av);
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

typedef FLOAT MAT4[4][4];

#define MAT4IDENT { {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, \
                {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0} }

extern MAT4 GLOBAL_mgf_m4Ident;

// Regular transformation
class XF {
  public:
    MAT4 xfm; // Transform matrix
    FLOAT sca; // Scale factor
};

// Maximum array dimensions
#define XF_MAXDIM 8

class XfArrayArg {
  public:
    short i; // Current count
    short n; // Current maximum
    char arg[8]; // String argument value
};

class XfArray {
  public:
    MgdReaderFilePosition spos; // Starting position on input
    int ndim; // Number of array dimensions
    XfArrayArg aarg[XF_MAXDIM];
};

class XfSpec {
public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    XF xf; // Cumulative transformation
    XfArray *xarr; // Transformation array pointer
    XfSpec *prev; // Previous transformation context
}; // Followed by argument buffer

extern XfSpec *GLOBAL_mgf_xfContext; // Current transform context
extern char **GLOBAL_mgf_xfLastTransform; // Last transform argument

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

extern int handleTransformationEntity(int ac, char **av); // Handle xf entity
extern void mgfTransformPoint(FVECT v1, FVECT v2); // Transform point
extern void mgfTransformVector(FVECT v1, FVECT v2); // Transform vector

#endif
