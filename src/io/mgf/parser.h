#ifndef __MGF_PARSER__
#define __MGF_PARSER__

#include "io/mgf/MgfContext.h"

extern int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv, MgfContext *context);
extern int mgfDefaultHandlerForUnknownEntities(int ac, char **av, MgfContext *context);
extern unsigned GLOBAL_mgf_unknownEntitiesCounter;

/**
The general process for running the parser is to fill in the handleCallbacks
array with handlers for each entity you know how to handle.
Then, call mg_init to fill in the rest.  This function will report
an error and quit if you try to support an inconsistent set of entities.
For each file you want to parse, call mgfLoad with the file name.
To read from standard input, use nullptr as the file name.
For additional control over error reporting and file management,
use mgfOpen, mgfReadNextLine, mgfParseCurrentLine and mgfClose instead of mgfLoad.
To globalPass an entity of your own construction to the parser, use
the mgfHandle function rather than the handleCallbacks routines directly.
(The first argument to mgfHandle is the entity #, or -1.)
To free any data structures and clear the parser, use mgfClear.
If there is an error, mgfLoad, mgfOpen, mgfParseCurrentLine, mgfHandle and
mgfGoToFilePosition will return an error from the list above.  In addition,
mgfLoad will report the error to stderr. The mgfReadNextLine routine
returns 0 when the end of file has been reached.
*/

extern int mgfReadNextLine(MgfContext *context);
extern int mgfParseCurrentLine(MgfContext *context);
extern void mgfClear(MgfContext *context);

/**
Definitions for context handling routines (materials, colors, vectors)
*/

extern int handleColorEntity(int ac, char **av, MgfContext * /*context*/);
extern int handleVertexEntity(int ac, char **av, MgfContext * /*context*/);
extern void initColorContextTables();

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
