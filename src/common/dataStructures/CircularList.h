/**
Implementation is based on Stroustrup 'The C++ Programming Language' Section 8.3

Intrusive (Data contains next field) and Non-intrusive lists are possible. Iterators are available.
*/

#ifndef __CIRCULAR_LISTS__
#define __CIRCULAR_LISTS__

template<class T> class CTSList_Iter;

class CISLink {
  public:
    CISLink *nextLink;

    CISLink();
};

class CircularListBase {
  private:
    CISLink *last;

  public:
    CircularListBase();
    virtual ~CircularListBase() {};
    virtual void add(CISLink *data);
    virtual void append(CISLink *data);
    CISLink *remove();
    virtual void clear();

    friend class CircularListBaseIterator;
};

class CircularListBaseIterator {
  private:
    CISLink *currentElement;
    CircularListBase *currentList;

  public:
    virtual ~CircularListBaseIterator() {};
    explicit CircularListBaseIterator(CircularListBase &list);
    virtual CISLink *next();
    void init(CircularListBase &list);
};

template<class T>
class CTSLink : public CISLink {
  public:
    T data;
    explicit CTSLink(const T &data);
};

template<class T>
CTSLink<T>::CTSLink(const T &data) : data(data) {
}

template<class T>
class CTSList : protected CircularListBase {
  public:
    ~CTSList() override;
    virtual void add(const T &data);
    virtual void append(const T &data);
    void removeAll();
    void clear() override;

    friend class CTSList_Iter<T>;
};

template<class T>
inline CTSList<T>::~CTSList() {
}

template<class T>
inline void CTSList<T>::add(const T &data) {
    CircularListBase::add(new CTSLink<T>(data));
}

template<class T>
inline void CTSList<T>::append(const T &data) {
    CircularListBase::append(new CTSLink<T>(data));
}

template<class T>
inline void CTSList<T>::removeAll() {
    CTSLink<T> *link = (CTSLink<T> *) CircularListBase::remove();

    while ( link != nullptr ) {
        delete link;
        link = (CTSLink<T> *) CircularListBase::remove();
    }
}

template<class T>
inline void CTSList<T>::clear() {
    CircularListBase::clear();
}

/**
For the non-intrusive list
*/
template<class T>
class CTSList_Iter final : private CircularListBaseIterator {
public:
    explicit CTSList_Iter(CTSList<T> &list);
    ~CTSList_Iter() final {};
    T *nextOnSequence();
    void init(CTSList<T> &list);
};

template<class T>
CTSList_Iter<T>::CTSList_Iter(CTSList<T> &list) : CircularListBaseIterator(list) {
}

template<class T>
inline T *CTSList_Iter<T>::nextOnSequence() {
    CTSLink<T> *link = (CTSLink<T> *) CircularListBaseIterator::next();
    return (link ? &link->data : nullptr);
}

template<class T>
inline void CTSList_Iter<T>::init(CTSList<T> &list) {
    CircularListBaseIterator::init(list);
}

#endif
