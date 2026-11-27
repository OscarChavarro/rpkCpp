#ifndef __ENTITY_LISTENER__
#define __ENTITY_LISTENER__

#include "vsdk/toolkit/common/EntityEvent.h"

/**
Subscriber of the events emitted by an `Entity`. Objects that depend on an
entity (editors, views, caches...) implement this interface and subscribe
with `Entity::addEntityListener` to learn when it changes or is discarded.
*/
class EntityListener {
public:
    virtual ~EntityListener() {}

    /**
    Called by the entity each time it emits an event.
    @param event the event, with its source entity and type
    */
    virtual void notifyEntityEvent(const EntityEvent &event) = 0;
};

#endif
