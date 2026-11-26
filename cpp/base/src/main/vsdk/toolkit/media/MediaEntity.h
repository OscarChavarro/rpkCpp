#ifndef __MEDIA_ENTITY__
#define __MEDIA_ENTITY__

#include "vsdk/toolkit/common/Entity.h"

/**
A `MediaEntity` in VitralSDK is a software element with data
structures associated with multimedia information, and some minor basic
algorithms for supporting them.

The MediaEntity abstract class provides an interface for multimedia related
classes NOT related with 3D geometry. This serves two purposes:
  - To help in design level organization of multimedia related classes (this
    eases the study of the class hierarchy)
  - To provide a place to locate possible future operations, common to all
    multimedia objects. Note that currently none of such operations have been
    detected.
*/
class MediaEntity : public Entity {

public:
    virtual ~MediaEntity() = default;

};

#endif
