#ifndef __JAVA_COMPARATOR__
#define __JAVA_COMPARATOR__

namespace java {

/**
@return true when `a` goes before `b` in the natural order of their type
(its `operator<`, the order of the `java.lang.Comparable` emulations)
*/
template <class A, class B>
inline bool
naturalOrderLess(const A &a, const B &b)
{
    return a < b;
}

/**
Emulation of `java.util.Comparator`: a total order over the elements of a
collection, used by `ArrayList::sort`. Lambdas returning an `int` with the
same convention can be given wherever a comparator is expected.
*/
template <class T>
class Comparator {
  public:
    virtual ~Comparator() {}

    /**
    @return a negative number, zero or a positive number when `a` goes
    before, is equivalent to or goes after `b`
    */
    virtual int compare(const T &a, const T &b) const = 0;

    int
    operator()(const T &a, const T &b) const
    {
        return compare(a, b);
    }
};

/**
Emulation of `java.util.Comparator.naturalOrder()`: the order of the
`operator<` of the elements.
*/
template <class T>
class NaturalOrderComparator : public Comparator<T> {
  public:
    int
    compare(const T &a, const T &b) const override
    {
        if ( naturalOrderLess(a, b) ) {
            return -1;
        }
        if ( naturalOrderLess(b, a) ) {
            return 1;
        }
        return 0;
    }
};

}

#endif
