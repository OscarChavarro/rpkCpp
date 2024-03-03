/**
Implementation is based on Stroustrup 'The C++ Prog Language'
Section 8.3

Intrusive (Data contains next field) and Non-intrusive lists
are possible. Iterators are available.
*/

#ifndef __CSLIST__
#define __CSLIST__

#include <cstdlib>
#include <cstdio>

class CSList_Base_Iter;

template<class T>
class CISList_Iter;

template<class T>
class CTSList_Iter;

class CISLink {
  public:
    CISLink *m_Next;

    CISLink() { m_Next = nullptr; }
    CISLink(CISLink *p) { m_Next = p; }
};

template<class T>
class CTSLink : public CISLink {
public:
    T m_Data;
    CTSLink(const T &data) : m_Data(data) {}
};


class CircularListBase {
  private:
    CISLink *m_Last;

    /*** Methods ***/

public:
    void add(CISLink *data);
    void Append(CISLink *link);
    CISLink *Remove();
    void clear() { m_Last = nullptr; }
    CircularListBase() { m_Last = nullptr; }
    friend class CSList_Base_Iter;
};

/* Implementation of base functions */
inline void
CircularListBase::add(CISLink *data) {
    // Add an element to the head of the list

    if ( m_Last != nullptr ) {
        // Not empty
        data->m_Next = m_Last->m_Next;
    } else {
        m_Last = data;
    }

    m_Last->m_Next = data;
}

inline void
CircularListBase::Append(CISLink *data) {
    if ( m_Last != nullptr ) {
        data->m_Next = m_Last->m_Next;
        m_Last = m_Last->m_Next = data;
    } else {
        m_Last = data->m_Next = data;
    }
}

/*** Definition & Implementation of an intrusive list ***/
template<class T>
class CircularList : private CircularListBase {
public:
    void add(T *data) { CircularListBase::add(data); }
    void clear() { CircularListBase::clear(); }
    friend class CISList_Iter<T>;
};

/*** Definition & Implementation of a non-intrusive list ***/
template<class T>
class CTSList : protected CircularListBase {
  public:
    void add(const T &data);
    void append(const T &data);
    void removeAll();
    void clear() { CircularListBase::clear(); }
    friend class CTSList_Iter<T>;
};

template<class T>
inline void CTSList<T>::add(const T &data) {
    CircularListBase::add(new CTSLink<T>(data));
}

template<class T>
inline void CTSList<T>::append(const T &data) {
    CircularListBase::Append(new CTSLink<T>(data));
}

template<class T>
inline void CTSList<T>::removeAll() {
    CTSLink<T> *link = (CTSLink<T> *) CircularListBase::Remove();

    while ( link != nullptr ) {
        delete link;
        link = (CTSLink<T> *) CircularListBase::Remove();
    }
}

/*** Iterators for the different list classes ***/

/* For the base class - don't use directly */
class CSList_Base_Iter {
private:
    CISLink *m_CurrentElement;
    CircularListBase *m_CurrentList;
public:
    inline CSList_Base_Iter(CircularListBase &list);
    inline CISLink *Next();
    inline void Init(CircularListBase &list);
};

inline void CSList_Base_Iter::Init(CircularListBase &list) {
    m_CurrentList = &list;
    m_CurrentElement = m_CurrentList->m_Last;
}

inline CSList_Base_Iter::CSList_Base_Iter(CircularListBase &list) {
    Init(list);
}

inline CISLink *CSList_Base_Iter::Next() {
    CISLink *ret = (m_CurrentElement ?
                    (m_CurrentElement = m_CurrentElement->m_Next) :
                    nullptr);
    if ( m_CurrentElement == m_CurrentList->m_Last ) {
        m_CurrentElement = nullptr;
    }

    return ret;
}

/* For the intrusive list */
template<class T>
class CISList_Iter : private CSList_Base_Iter {
public:
    T *Next() { return (T *) CSList_Base_Iter::Next(); }
    void Init(CircularList<T> &list) { CSList_Base_Iter::Init(list); }
};

/* For the non-intrusive list */
template<class T>
class CTSList_Iter : private CSList_Base_Iter {
public:
    CTSList_Iter(CTSList<T> &list) : CSList_Base_Iter(list) {}
    inline T *Next();
    inline void Init(CTSList<T> &list);
};

template<class T>
inline T *CTSList_Iter<T>::Next() {
    CTSLink<T> *link = (CTSLink<T> *) CSList_Base_Iter::Next();
    return (link ? &link->m_Data : nullptr);
}

template<class T>
inline void CTSList_Iter<T>::Init(CTSList<T> &list) {
    CSList_Base_Iter::Init(list);
}

#endif
