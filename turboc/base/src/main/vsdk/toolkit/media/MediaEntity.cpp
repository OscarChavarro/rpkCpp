#include "vsdk/toolkit/media/MediaEntity.h"

const Class MediaEntity::CLASS = { "MediaEntity", &Entity::CLASS };

const Class *
MediaEntity::getClass() const
{
    return &CLASS;
}
