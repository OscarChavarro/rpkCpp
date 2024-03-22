#ifndef MGF_MAJOR_VERSION_NUMBER

#include "common/mymath.h"
#include "skin/RadianceMethod.h"
#include "io/mgf/Vector3Dd.h"
#include "io/mgf/MgfColorContext.h"
#include "io/mgf/mgfHandlerMaterial.h"
#include "io/mgf/MgfVertexContext.h"
#include "io/mgf/mgfDefinitions.h"

// Major version number
#define MGF_MAJOR_VERSION_NUMBER 2

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

extern int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, RadianceMethod *context);
extern int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv);
extern int mgfDefaultHandlerForUnknownEntities(int ac, char **av);
extern unsigned GLOBAL_mgf_unknownEntitiesCounter;

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

extern MgfReaderContext *GLOBAL_mgf_file;
extern int GLOBAL_mgf_divisionsPerQuarterCircle;

extern void mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **, RadianceMethod *));
extern int mgfOpen(MgfReaderContext *, char *);
extern int mgfReadNextLine();
extern int mgfParseCurrentLine(RadianceMethod *context);
extern void mgfClose();
extern void mgfClear();

extern int handleIncludedFile(int ac, char **av, RadianceMethod *context);

/**
Definitions for context handling routines (materials, colors, vectors)
*/

extern int handleColorEntity(int ac, char **av, RadianceMethod * /*context*/);
extern int handleVertexEntity(int ac, char **av, RadianceMethod * /*context*/);
extern void clearContextTables();

/**
Definitions for hierarchical object name handler
*/

extern int GLOBAL_mgf_objectNames; // Depth of name hierarchy
extern char **GLOBAL_mgf_objectNamesList; // Names in hierarchy

/**
Definitions for hierarchical transformation handler
*/

extern char **GLOBAL_mgf_xfLastTransform; // Last transform argument

#endif
