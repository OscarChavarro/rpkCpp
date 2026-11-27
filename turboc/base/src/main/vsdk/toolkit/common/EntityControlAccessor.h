#ifndef __ENTITY_CONTROL_ACCESSOR__
#define __ENTITY_CONTROL_ACCESSOR__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Class.h"
#include "java/lang/String.h"

class Entity;

/**
C++ replacement of the Java reflection used by generic editors: the pair of
accessors (getter and setter) of one editable attribute of an `Entity`,
declared together with its control specification (see
`Entity::addControlSpecification`).

In Java, a generic editor finds `getRadius()` and `setRadius(double)` from
the name "radius" of the specification "double;radius;(0, INFINITE)". C++
has no such reflection, so each entity class registers, in its
constructor, pointers to its accessor member functions; this class keeps
them behind an interface that works with the attribute values as text, the
way GUI editors present them.
*/
class EntityControlAccessor {
private:
    String name;

public:
    /**
    @param name attribute name, as in the control specification (i.e.
    "radius")
    */
    EntityControlAccessor() : name("") {}
    explicit EntityControlAccessor(const String &name) : name(name) {}
    virtual ~EntityControlAccessor() {}

    /**
    @return the attribute name, as in the control specification
    */
    const String &getName() const { return name; }

    /**
    @param name attribute name, as in the control specification (set by
    `Entity::addControlSpecification`)
    */
    void setName(const String &name) { this->name = name; }

    /**
    @return a copy of this accessor, for the copies of the entity
    */
    virtual EntityControlAccessor *clone() const = 0;

    /**
    @return the type of the accessors as written in control specifications:
    "double", "float", "int" or "long"
    */
    virtual const char *getType() const = 0;

    /**
    Reads the attribute through the getter, as Java `String.valueOf` writes
    it.
    @param entity entity to read; must be of the class that registered the
    accessor
    @param outText the value as text
    @return false if the entity is not of the class of the accessor
    */
    virtual BOOLEAN readValue(const Entity &entity,
                           String &outText) const = 0;

    /**
    Parses a value of the type of the accessor, without writing it.
    @param text value as typed by the user, without surrounding blanks
    @return the value, as a double to check it against the valid interval
    @throws java::NumberFormatException if the text is not a valid value of
    the type
    */
    virtual double parseValue(const String &text) const = 0;

    /**
    Parses a value and writes it to the attribute through the setter.
    @param entity entity to change; must be of the class that registered
    the accessor
    @param text value as typed by the user, without surrounding blanks
    @return false if the entity is not of the class of the accessor
    @throws java::NumberFormatException if the text is not a valid value of
    the type
    */
    virtual BOOLEAN writeValue(Entity &entity,
                            const String &text) const = 0;

protected:
    /**
    @return true if the entity is an instance of the class (Java
    `instanceof`; defined where `Entity` is complete)
    */
    static BOOLEAN isInstance(const Entity &entity, const Class &type);
};

/**
Conversions between text and the value types supported by control
specifications. Only the overloads below exist, so registering accessors
of other types does not compile.

C++ port note: overloads selected by the type of the value (or of a
pointer to it) replace the specializations of a class template, which
legacy compilers (gcc 2.7, Turbo C++ 3.0) do not have.
*/
class EntityControlValue {
public:
    static const char *getType(const double *) { return "double"; }
    static const char *getType(const float *) { return "float"; }
    static const char *getType(const int *) { return "int"; }
    /// Java long is 64 bits: C++ `long long` (and `long` on LP64 platforms)
#ifdef VSDK_HAS_LONG_LONG
    static const char *getType(const long long *) { return "long"; }
#endif
    static const char *getType(const long *) { return "long"; }

    static void parse(const String &text, double *outValue);
    static void parse(const String &text, float *outValue);
    static void parse(const String &text, int *outValue);
#ifdef VSDK_HAS_LONG_LONG
    static void parse(const String &text, long long *outValue);
#endif
    static void parse(const String &text, long *outValue);

    static String toString(double value);
    static String toString(float value);
    static String toString(int value);
#ifdef VSDK_HAS_LONG_LONG
    static String toString(long long value);
#endif
    static String toString(long value);

private:
    EntityControlValue();
};

/**
Accessor pair given as pointers to member functions of the entity class
`T`, for an attribute of type `V`, i.e. `&Sphere::getRadius` and
`&Sphere::setRadius`. The getter may be const or not.
*/
template <class T, class V>
class MemberEntityControlAccessor : public EntityControlAccessor {
public:
    typedef V (T::*ConstGetter)() const;
    typedef V (T::*Getter)();
    typedef void (T::*Setter)(V);

private:
    ConstGetter constGetter;
    Getter getter;
    Setter setter;

    V get(const T &entity) const
    {
        if ( constGetter != 0 ) {
            return (entity.*constGetter)();
        }
        return (((T &)(entity)).*getter)();
    }

public:
    /**
    The name is given by `Entity::addControlSpecification`.
    */
    MemberEntityControlAccessor(ConstGetter getter, Setter setter)
        : EntityControlAccessor(), constGetter(getter),
          getter(0), setter(setter) {}

    MemberEntityControlAccessor(Getter getter, Setter setter)
        : EntityControlAccessor(), constGetter(0),
          getter(getter), setter(setter) {}

    virtual EntityControlAccessor *clone() const
    {
        return new MemberEntityControlAccessor<T, V>(*this);
    }

    virtual const char *getType() const
    {
        return EntityControlValue::getType((const V *)NULL);
    }

    virtual BOOLEAN readValue(const Entity &entity,
                           String &outText) const
    {
        if ( !isInstance(entity, T::CLASS) ) {
            return FALSE;
        }
        const T *target = (const T *)&entity;
        outText = EntityControlValue::toString(get(*target));
        return TRUE;
    }

    virtual double parseValue(const String &text) const
    {
        V value;
        EntityControlValue::parse(text, &value);
        return ((double)(value));
    }

    virtual BOOLEAN writeValue(Entity &entity,
                            const String &text) const
    {
        if ( !isInstance(entity, T::CLASS) ) {
            return FALSE;
        }
        T *target = (T *)&entity;
        V value;
        EntityControlValue::parse(text, &value);
        (target->*setter)(value);
        return TRUE;
    }
};

#endif
