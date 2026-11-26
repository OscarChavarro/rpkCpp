#ifndef __ENTITY_CONTROL_ACCESSOR__
#define __ENTITY_CONTROL_ACCESSOR__

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
    java::String name;

public:
    /**
    @param name attribute name, as in the control specification (i.e.
    "radius")
    */
    explicit EntityControlAccessor(const java::String &name) : name(name) {}
    virtual ~EntityControlAccessor() {}

    /**
    @return the attribute name, as in the control specification
    */
    const java::String &getName() const { return name; }

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
    virtual bool readValue(const Entity &entity,
                           java::String &outText) const = 0;

    /**
    Parses a value of the type of the accessor, without writing it.
    @param text value as typed by the user, without surrounding blanks
    @return the value, as a double to check it against the valid interval
    @throws java::NumberFormatException if the text is not a valid value of
    the type
    */
    virtual double parseValue(const java::String &text) const = 0;

    /**
    Parses a value and writes it to the attribute through the setter.
    @param entity entity to change; must be of the class that registered
    the accessor
    @param text value as typed by the user, without surrounding blanks
    @return false if the entity is not of the class of the accessor
    @throws java::NumberFormatException if the text is not a valid value of
    the type
    */
    virtual bool writeValue(Entity &entity,
                            const java::String &text) const = 0;
};

/**
Conversions between text and the value types supported by control
specifications. Only the specializations below exist, so registering
accessors of other types does not compile.
*/
template <class V> class EntityControlValue;

template <> class EntityControlValue<double> {
public:
    static const char *getType() { return "double"; }
    static double parse(const java::String &text);
    static java::String toString(double value);
};

template <> class EntityControlValue<float> {
public:
    static const char *getType() { return "float"; }
    static float parse(const java::String &text);
    static java::String toString(float value);
};

template <> class EntityControlValue<int> {
public:
    static const char *getType() { return "int"; }
    static int parse(const java::String &text);
    static java::String toString(int value);
};

/// Java long is 64 bits: C++ `long long` (and `long` on LP64 platforms)
template <> class EntityControlValue<long long> {
public:
    static const char *getType() { return "long"; }
    static long long parse(const java::String &text);
    static java::String toString(long long value);
};

template <> class EntityControlValue<long> {
public:
    static const char *getType() { return "long"; }
    static long parse(const java::String &text);
    static java::String toString(long value);
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
        if ( constGetter != nullptr ) {
            return (entity.*constGetter)();
        }
        return (const_cast<T &>(entity).*getter)();
    }

public:
    MemberEntityControlAccessor(const java::String &name,
                                ConstGetter getter, Setter setter)
        : EntityControlAccessor(name), constGetter(getter),
          getter(nullptr), setter(setter) {}

    MemberEntityControlAccessor(const java::String &name,
                                Getter getter, Setter setter)
        : EntityControlAccessor(name), constGetter(nullptr),
          getter(getter), setter(setter) {}

    virtual EntityControlAccessor *clone() const override
    {
        return new MemberEntityControlAccessor<T, V>(*this);
    }

    virtual const char *getType() const override
    {
        return EntityControlValue<V>::getType();
    }

    virtual bool readValue(const Entity &entity,
                           java::String &outText) const override
    {
        const T *target = dynamic_cast<const T *>(&entity);
        if ( target == nullptr ) {
            return false;
        }
        outText = EntityControlValue<V>::toString(get(*target));
        return true;
    }

    virtual double parseValue(const java::String &text) const override
    {
        return static_cast<double>(EntityControlValue<V>::parse(text));
    }

    virtual bool writeValue(Entity &entity,
                            const java::String &text) const override
    {
        T *target = dynamic_cast<T *>(&entity);
        if ( target == nullptr ) {
            return false;
        }
        V value = EntityControlValue<V>::parse(text);
        (target->*setter)(value);
        return true;
    }
};

#endif
