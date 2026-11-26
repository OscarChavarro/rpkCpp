#include "java/lang/String.h"
#include <cctype>
#include <cstdlib>
#include <typeinfo>
#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/common/EntityListener.h"

Entity::Entity() : controlSpecifications(nullptr), controlAccessors(nullptr),
    entityListeners(nullptr)
{
}

Entity::Entity(const Entity &other) :
    controlSpecifications(nullptr), controlAccessors(nullptr),
    entityListeners(nullptr)
{
    if ( other.controlSpecifications != nullptr ) {
        controlSpecifications = new java::ArrayList<java::String>(
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
    controlSpecifications = nullptr;
    if ( other.controlSpecifications != nullptr ) {
        controlSpecifications = new java::ArrayList<java::String>(
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
    if ( other.controlAccessors == nullptr ) {
        return;
    }
    for ( long int i = 0; i < other.controlAccessors->size(); i++ ) {
        addControlAccessor(other.controlAccessors->get(i)->clone());
    }
}

void
Entity::deleteControlAccessors()
{
    if ( controlAccessors == nullptr ) {
        return;
    }
    for ( long int i = 0; i < controlAccessors->size(); i++ ) {
        delete controlAccessors->get(i);
    }
    delete controlAccessors;
    controlAccessors = nullptr;
}

void
Entity::addControlAccessor(EntityControlAccessor *accessor)
{
    if ( controlAccessors == nullptr ) {
        controlAccessors = new java::ArrayList<EntityControlAccessor *>();
    }
    controlAccessors->add(accessor);
}

/**
@return the name field of a "type;name;interval" specification, without
surrounding blanks, or an empty string if it has none (the generic editors
report malformed specifications when they parse them)
*/
java::String
Entity::controlNameOf(const java::String &specification)
{
    java::String text(specification.c_str());
    size_t begin = text.find(';');
    if ( begin == -1 ) {
        return java::String();
    }
    begin++;
    size_t end = text.find(';', begin);
    if ( end == -1 ) {
        end = text.size();
    }
    while ( begin < end && std::isspace((unsigned char)text[begin]) ) {
        begin++;
    }
    while ( end > begin && std::isspace((unsigned char)text[end - 1]) ) {
        end--;
    }
    return java::String(text.substr(begin, end - begin).c_str());
}

const EntityControlAccessor *
Entity::getControlAccessor(const java::String &name) const
{
    if ( controlAccessors == nullptr ) {
        return nullptr;
    }
    for ( long int i = 0; i < controlAccessors->size(); i++ ) {
        if ( controlAccessors->get(i)->getName() == name ) {
            return controlAccessors->get(i);
        }
    }
    return nullptr;
}

java::ArrayList<java::String> &
Entity::getControlSpecifications()
{
    if ( controlSpecifications == nullptr ) {
        controlSpecifications = new java::ArrayList<java::String>();
    }
    return *controlSpecifications;
}

void
Entity::setControlSpecifications(
    const java::ArrayList<java::String> &controlSpecifications)
{
    getControlSpecifications() = controlSpecifications;
}

void
Entity::addEntityListener(EntityListener *listener)
{
    if ( listener == nullptr ) {
        return;
    }
    if ( entityListeners == nullptr ) {
        entityListeners = new java::ArrayList<EntityListener *>();
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
    if ( entityListeners == nullptr ) {
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
    entityListeners = nullptr;
}

void
Entity::fireEntityEvent(EntityEvent::Type type)
{
    if ( entityListeners == nullptr || entityListeners->size() == 0 ) {
        return;
    }
    EntityEvent event(this, type);
    // Copy: listeners may unsubscribe while being notified
    java::ArrayList<EntityListener *> listeners(*entityListeners);
    for ( long int i = 0; i < listeners.size(); i++ ) {
        listeners.get(i)->notifyEntityEvent(event);
    }
}

java::String
Entity::getClassSimpleName() const
{
    const char *mangled = typeid(*this).name();
    java::String name(mangled);
#if defined(__GNUC__) || defined(__clang__)
    int status = 0;
    char *demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    if ( status == 0 && demangled != nullptr ) {
        name = demangled;
    }
    std::free(demangled);
#else
    // MSVC names are "class Sphere"
    size_t space = name.rfind(' ');
    if ( space != -1 ) {
        name = name.substr(space + 1);
    }
#endif
    size_t scope = name.rfind("::");
    if ( scope != -1 ) {
        name = name.substr(scope + 2);
    }
    return java::String(name.c_str());
}
