#ifndef __ENTITY__
#define __ENTITY__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/EntityControlAccessor.h"
#include "vsdk/toolkit/common/EntityEvent.h"

class EntityListener;

/**
This class is a base superclass for all classes in the VSDK model (as of
Vitral applications are based upon Model-View-Controller or MVC design
pattern).

Besides the memory accounting of `getSizeInBytes`, each entity keeps:
  - A list of control specifications ("type;name;valid interval"), used by
    generic editors to build GUI dialogs for the entity, and the accessors
    (pointers to getter and setter member functions) of each specified
    attribute: the C++ replacement of the Java reflection that finds
    `getRadius()` / `setRadius(double)` from the name "radius". Subclasses
    declare both at once with `addControlSpecification`.
  - A list of subscribers (`EntityListener`), notified with `update()` when
    the entity changes and with `dispose()` when it is discarded. The
    destructor calls `dispose()`, so deleting an entity notifies its
    subscribers.
*/
class Entity {
private:
    /// Created on first use, so entities without them pay only a pointer
    java::ArrayList<java::String> *controlSpecifications;
    /// Accessors of the specified attributes (owned), created on first use
    java::ArrayList<EntityControlAccessor *> *controlAccessors;
    /// Created on first subscription; never copied with the entity
    java::ArrayList<EntityListener *> *entityListeners;

    void fireEntityEvent(EntityEvent::Type type);
    void copyControlAccessors(const Entity &other);
    void deleteControlAccessors();
    void addControlAccessor(EntityControlAccessor *accessor);
    static java::String controlNameOf(const java::String &specification);

public:
    /// Constants used for operations of type getSizeInBytes
    static const int BYTE_SIZE_IN_BYTES = 1;
    static const int INT_SIZE_IN_BYTES = 4;
    static const int LONG_SIZE_IN_BYTES = 8;
    static const int FLOAT_SIZE_IN_BYTES = 4;
    static const int DOUBLE_SIZE_IN_BYTES = 8;
    static const int VECTOR3D_SIZE_IN_BYTES = 24;
    static const int COLORRGB_SIZE_IN_BYTES = 24;
    static const int POINTER_SIZE_IN_BYTES = 8;

    Entity();

    /**
    Copies the control specifications; subscribers are not copied, as they
    subscribed to the original entity.
    */
    Entity(const Entity &other);
    Entity &operator=(const Entity &other);

    /**
    Emits `DELETED` to the remaining subscribers (see `dispose()`).
    */
    virtual ~Entity();

    /**
    @return the number of bytes current object ocupies in RAM when loaded
    */
    virtual int getSizeInBytes() const {
        return 0;
    }

    /**
    Returns the control specifications used by reflection-based generic
    editors, creating an empty list on first access.
    @return the mutable list of control specifications for this entity
    */
    java::ArrayList<java::String> &getControlSpecifications();

    /**
    Replaces the control specifications used by reflection-based generic
    editors.
    @param controlSpecifications new list of control specifications
    */
    void setControlSpecifications(
        const java::ArrayList<java::String> &controlSpecifications);

    /**
    Declares an editable attribute: adds its control specification and
    registers its accessors, so generic editors can read and write it. Call
    it from the constructor of the entity class, i.e.
    <pre>
    addControlSpecification("double;radius;(0, INFINITE)",
        &Sphere::getRadius, &Sphere::setRadius);
    </pre>
    @param specification control specification in "type;name;interval"
    format; its type must be the one of the accessors
    @param getter member function returning the attribute (const or not)
    @param setter member function changing the attribute
    */
    template <class T, class V>
    void addControlSpecification(const java::String &specification,
                                 V (T::*getter)() const,
                                 void (T::*setter)(V))
    {
        getControlSpecifications().add(specification);
        addControlAccessor(new MemberEntityControlAccessor<T, V>(
            controlNameOf(specification), getter, setter));
    }

    template <class T, class V>
    void addControlSpecification(const java::String &specification,
                                 V (T::*getter)(),
                                 void (T::*setter)(V))
    {
        getControlSpecifications().add(specification);
        addControlAccessor(new MemberEntityControlAccessor<T, V>(
            controlNameOf(specification), getter, setter));
    }

    /**
    @param name attribute name, as in the control specification (i.e.
    "radius")
    @return the accessors registered for the attribute, or null if the
    entity class registered none
    */
    const EntityControlAccessor *getControlAccessor(
        const java::String &name) const;

    /**
    @return the name of the class of this entity, without namespaces (as
    Java `getClass().getSimpleName()`); while it is destroyed, the one of
    the classes not destroyed yet
    */
    java::String getClassSimpleName() const;

    /**
    Subscribes a listener to the events of this entity. Adding the same
    listener twice has no effect.
    @param listener object to notify; null is ignored
    */
    void addEntityListener(EntityListener *listener);

    /**
    Unsubscribes a listener from the events of this entity.
    @param listener object to stop notifying
    */
    void removeEntityListener(EntityListener *listener);

    /**
    Notifies the subscribers that this entity changed. Code that modifies an
    entity (editors, tools) calls this method after the change, so the
    objects that depend on the entity can refresh.
    */
    virtual void update();

    /**
    Notifies the subscribers that this entity was discarded and then drops
    all of them. The destructor calls it; code that removes an entity from a
    model without destroying it (i.e. to allow undo) can call it directly.
    */
    virtual void dispose();
};

#endif
