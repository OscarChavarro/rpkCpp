#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include <cstdio>

#include "io/context/BaseContext.h"

class MgfReaderFilePosition;

enum class MgfHandlerType {
    DISCARD_UNNEEDED,
    INCLUDE_FILE,
    ENTITY_SPHERE,
    ENTITY_TORUS,
    ENTITY_CYLINDER,
    ENTITY_RING,
    ENTITY_CONE,
    ENTITY_PRISM,
    ENTITY_FACE_WITH_HOLES,
    COLOR_SPEC_HELPER,
    COLOR_MIX_HELPER,
    COLOR_TEMPERATURE_HELPER,
    HANDLE_VERTEX,
    HANDLE_FACE,
    HANDLE_FACE_WITH_HOLES,
    HANDLE_SURFACE,
    HANDLE_COLOR,
    HANDLE_MATERIAL,
    HANDLE_TRANSFORM,
    HANDLE_OBJECT
};

class MgfEntityHandler {
  public:
    virtual int handle(int argc, const char **argv, BaseContext *context) const = 0;
    virtual MgfHandlerType type() const = 0;
    virtual ~MgfEntityHandler() {}
};

extern MgfEntityHandler *mgfHandlerFromType(MgfHandlerType handlerType);
extern bool mgfHandlerMatches(const MgfEntityHandler *handler, MgfHandlerType handlerType);

extern int mgfOpen(ReaderContext *readerContext, const char *functionCallback, BaseContext *context);
extern void mgfClose(BaseContext *context);
extern void doError(const char *errmsg, BaseContext *context);
extern void doWarning(const char *errmsg, BaseContext *context);
extern void mgfGetFilePosition(MgfReaderFilePosition *pos, BaseContext *context);
extern int mgfGoToFilePosition(const MgfReaderFilePosition *pos, BaseContext *context);
extern int mgfEntity(const char *name, BaseContext *context);
extern int mgfHandle(int entityIndex, int argc, const char **argv, BaseContext * /*context*/);
extern void mgfLookUpFreeMemory();

#include "io/context/TransformContext.h"

#endif
