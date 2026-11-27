#include <ctype.h>
#include <stdlib.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/common/EntityListener.h"

Entity::Entity() : controlSpecifications(NULL), controlAccessors(NULL),
    entityListeners(NULL)
{
}

Entity::Entity(const Entity &other) :
    controlSpecifications(NULL), controlAccessors(NULL),
    entityListeners(NULL)
{
    if ( other.controlSpecifications != NULL ) {
        controlSpecifications = new ArrayList<String>(
            *other.controlSpecifications);
    }
    copyControlAccessors(other);
}

Entity &
Entity::operator=(const Entity &other)
{
    if ( this == &other ) {
        return *this;
    }
    delete controlSpecifications;
    controlSpecifications = NULL;
    if ( other.controlSpecifications != NULL ) {
        controlSpecifications = new ArrayList<String>(
            *other.controlSpecifications);
    }
    deleteControlAccessors();
    copyControlAccessors(other);
    // Subscribers stay with this entity: they did not subscribe to `other`
    return *this;
}

Entity::~Entity()
{
    // Virtual calls inside a destructor resolve to Entity::dispose
    dispose();
    delete controlSpecifications;
    deleteControlAccessors();
}

void
Entity::copyControlAccessors(const Entity &other)
{
    if ( other.controlAccessors == NULL ) {
        return;
    }
    for ( long int i = 0; i < other.controlAccessors->size(); i++ ) {
        addControlAccessor(other.controlAccessors->get(i)->clone());
    }
}

void
Entity::deleteControlAccessors()
{
    if ( controlAccessors == NULL ) {
        return;
    }
    for ( long int i = 0; i < controlAccessors->size(); i++ ) {
        delete controlAccessors->get(i);
    }
    delete controlAccessors;
    controlAccessors = NULL;
}

void
Entity::addControlSpecification(const String &specification,
                                EntityControlAccessor *accessor)
{
    getControlSpecifications().add(specification);
    accessor->setName(controlNameOf(specification));
    addControlAccessor(accessor);
}

void
Entity::addControlAccessor(EntityControlAccessor *accessor)
{
    if ( controlAccessors == NULL ) {
        controlAccessors = new ArrayList<EntityControlAccessor *>();
    }
    controlAccessors->add(accessor);
}

/**
@return the name field of a "type;name;interval" specification, without
surrounding blanks, or an empty string if it has none (the generic editors
report malformed specifications when they parse them)
*/
String
Entity::controlNameOf(const String &specification)
{
    int begin = specification.indexOf(';');
    if ( begin < 0 ) {
        return String();
    }
    begin++;
    int end = specification.indexOf(';', begin);
    if ( end < 0 ) {
        end = specification.size();
    }
    while ( begin < end && isspace((unsigned char)specification[begin]) ) {
        begin++;
    }
    while ( end > begin && isspace((unsigned char)specification[end - 1]) ) {
        end--;
    }
    return specification.substring(begin, end);
}

const EntityControlAccessor *
Entity::getControlAccessor(const String &name) const
{
    if ( controlAccessors == NULL ) {
        return NULL;
    }
    for ( long int i = 0; i < controlAccessors->size(); i++ ) {
        if ( controlAccessors->get(i)->getName() == name ) {
            return controlAccessors->get(i);
        }
    }
    return NULL;
}

ArrayList<String> &
Entity::getControlSpecifications()
{
    if ( controlSpecifications == NULL ) {
        controlSpecifications = new ArrayList<String>();
    }
    return *controlSpecifications;
}

void
Entity::setControlSpecifications(
    const ArrayList<String> &controlSpecifications)
{
    getControlSpecifications() = controlSpecifications;
}

void
Entity::addEntityListener(EntityListener *listener)
{
    if ( listener == NULL ) {
        return;
    }
    if ( entityListeners == NULL ) {
        entityListeners = new ArrayList<EntityListener *>();
    }
    for ( long int i = 0; i < entityListeners->size(); i++ ) {
        if ( entityListeners->get(i) == listener ) {
            return;
        }
    }
    entityListeners->add(listener);
}

void
Entity::removeEntityListener(EntityListener *listener)
{
    if ( entityListeners == NULL ) {
        return;
    }
    for ( long int i = 0; i < entityListeners->size(); i++ ) {
        if ( entityListeners->get(i) == listener ) {
            entityListeners->remove(i);
            return;
        }
    }
}

void
Entity::update()
{
    fireEntityEvent(EntityEvent::UPDATED);
}

void
Entity::dispose()
{
    fireEntityEvent(EntityEvent::DELETED);
    delete entityListeners;
    entityListeners = NULL;
}

void
Entity::fireEntityEvent(EntityEvent::Type type)
{
    if ( entityListeners == NULL || entityListeners->size() == 0 ) {
        return;
    }
    EntityEvent event(this, type);
    // Copy: listeners may unsubscribe while being notified
    ArrayList<EntityListener *> listeners(*entityListeners);
    for ( long int i = 0; i < listeners.size(); i++ ) {
        listeners.get(i)->notifyEntityEvent(event);
    }
}

const Class Entity::CLASS = { "Entity", NULL };

const Class *
Entity::getClass() const
{
    return &CLASS;
}

BOOLEAN
Entity::instanceOf(const Class &type) const
{
    return type.isAssignableFrom(getClass());
}

String
Entity::getClassSimpleName() const
{
    return String(getClass()->getSimpleName());
}

// Integral constants bound to references (i.e. by `?:`) need a
// definition out of the class in C++98
