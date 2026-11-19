#ifndef __COMPARATOR__
#define __COMPARATOR__

template <class A, class B>
inline int naturalOrderValueLess(const A& a, const B& b)
{
    return a < b;
}

#ifdef __TURBOC__
/* Turbo compares only offsets with the built-in far-pointer operator<.
 * Ordered containers need a total order that also distinguishes selectors,
 * matching the pointer identity used by cpp's std::set/std::map. */
template <class T>
inline int naturalOrderValueLess(T* a, T* b)
{
    return (unsigned long)a < (unsigned long)b;
}
#endif

template <class A, class B>
#ifdef __TURBOC__
/* Materialize values before overloaded dispatch: TCC cannot deduce a
 * pointer through const T&, nor partially order two by-value templates. */
inline int naturalOrderLess(A a, B b)
#else
inline int naturalOrderLess(const A& a, const B& b)
#endif
{
    return naturalOrderValueLess(a, b);
}

/**
Emulation of `java.util.Comparator`: a total order over the elements of a
collection, used by `Collections::sort`.
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
};

/**
Emulation of `java.util.Comparator.naturalOrder()`: the order of the
`operator<` of the elements.
*/
template <class T>
class NaturalOrderComparator : public Comparator<T> {
  public:
    virtual int compare(const T &a, const T &b) const
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

#endif
