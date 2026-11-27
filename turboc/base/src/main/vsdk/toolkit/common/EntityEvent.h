#ifndef __ENTITY_EVENT__
#define __ENTITY_EVENT__

class Entity;

/**
Event emitted by an `Entity` to its subscribers (see
`Entity::addEntityListener`) when the entity changes or is discarded.
*/
class EntityEvent {
public:
    /**
    Kinds of events an entity can emit.
    */
    enum Type {
        /** Some attribute of the entity changed, see `Entity::update()` */
        UPDATED,
        /** The entity was discarded, see `Entity::dispose()` */
        DELETED
    };

private:
    Entity *source;
    Type type;

public:
    /**
    @param source entity that emits the event
    @param type kind of event
    */
    EntityEvent(Entity *source, Type type) : source(source), type(type) {}

    /**
    @return the entity that emitted the event
    */
    Entity *getSource() const { return source; }

    /**
    @return the kind of event
    */
    Type getType() const { return type; }
};

#endif
