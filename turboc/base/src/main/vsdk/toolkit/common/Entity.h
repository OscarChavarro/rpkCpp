#ifndef __ENTITY__
#define __ENTITY__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Class.h"
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
    ArrayList<String> *controlSpecifications;
    /// Accessors of the specified attributes (owned), created on first use
    ArrayList<EntityControlAccessor *> *controlAccessors;
    /// Created on first subscription; never copied with the entity
    ArrayList<EntityListener *> *entityListeners;

    void fireEntityEvent(EntityEvent::Type type);
    void copyControlAccessors(const Entity &other);
    void deleteControlAccessors();
    void addControlAccessor(EntityControlAccessor *accessor);
    static String controlNameOf(const String &specification);

public:
    /// Constants used for operations of type getSizeInBytes
    enum {
        BYTE_SIZE_IN_BYTES = 1,
        INT_SIZE_IN_BYTES = 4,
        LONG_SIZE_IN_BYTES = 8,
        FLOAT_SIZE_IN_BYTES = 4,
        DOUBLE_SIZE_IN_BYTES = 8,
        VECTOR3D_SIZE_IN_BYTES = 24,
        COLORRGB_SIZE_IN_BYTES = 24,
        POINTER_SIZE_IN_BYTES = 8
    };

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
    ArrayList<String> &getControlSpecifications();

    /**
    Replaces the control specifications used by reflection-based generic
    editors.
    @param controlSpecifications new list of control specifications
    */
    void setControlSpecifications(
        const ArrayList<String> &controlSpecifications);

    /**
    Declares an editable attribute: adds its control specification and
    registers its accessors, so generic editors can read and write it. Call
    it from the constructor of the entity class, i.e.
    <pre>
    MemberEntityControlAccessor<Sphere, double> *radiusAccessor =
        new MemberEntityControlAccessor<Sphere, double>(
            &Sphere::getRadius, &Sphere::setRadius);
    addControlSpecification("double;radius;(0, INFINITE)", radiusAccessor);
    </pre>
    (Turbo C++ 3.0 crashes with the new of a template class as an argument)
    C++ port note: the accessors are created by the caller, as legacy
    compilers (gcc 2.7, Turbo C++ 3.0) have no member templates.
    @param specification control specification in "type;name;interval"
    format; its type must be the one of the accessors
    @param accessor getter (const or not) and setter of the attribute, owned
    by this entity from now on; its name is taken from the specification
    */
    void addControlSpecification(const String &specification,
                                 EntityControlAccessor *accessor);

    /**
    @param name attribute name, as in the control specification (i.e.
    "radius")
    @return the accessors registered for the attribute, or null if the
    entity class registered none
    */
    const EntityControlAccessor *getControlAccessor(
        const String &name) const;

    /// Run time class descriptor (see `Class`)
    static const Class CLASS;

    /**
    @return the descriptor of the class of this object (Java `getClass()`)
    */
    virtual const Class *getClass() const;

    /**
    @return true if this object is an instance of the class or of one of
    its subclasses (Java `instanceof`)
    */
    BOOLEAN instanceOf(const Class &type) const;

    /**
    @return the name of the class of this entity, without namespaces (as
    Java `getClass().getSimpleName()`); while it is destroyed, the one of
    the classes not destroyed yet
    */
    String getClassSimpleName() const;

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
