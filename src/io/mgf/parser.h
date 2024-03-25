#ifndef __MGF_PARSER__
#define __MGF_PARSER__

#include "io/mgf/MgfContext.h"

extern int GLOBAL_mgf_objectNames; // Depth of name hierarchy
extern char **GLOBAL_mgf_objectNamesList; // Names in hierarchy
extern char **GLOBAL_mgf_xfLastTransform; // Last transform argument

extern int mgfReadNextLine(MgfContext *context);
extern int mgfParseCurrentLine(MgfContext *context);
extern void mgfClear(MgfContext *context);


#endif
