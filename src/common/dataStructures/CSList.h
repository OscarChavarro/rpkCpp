/* CSList.H : C++ template for single linked lists 
 *
 * Implementation is based on Stroustrup 'The C++ Prog Language'
 * Section 8.3
 *
 * Intrusive (Data contains next field) and Non-intrusive lists
 * are possible. Iterators are available.
 */

#ifndef _CSLIST_H_
#define _CSLIST_H_

#include <cstdlib>
#include <cstdio>

/*** Forward declarations ***/

class CSList_Base_Iter;

template<class T>
class CISList_Iter;

template<class T>
class CTSList_Iter;


/*** Definition of the links in the lists ***/

class CISLink {
  public:
    CISLink *m_Next;

    /*** Methods ***/

    CISLink() { m_Next = nullptr; }

    CISLink(CISLink *p) { m_Next = p; }
};


/* Class for a link in the non-intrusive list */

template<class T>
class CTSLink : public CISLink {
public:
    T m_Data;

    /*** Methods ***/

    /* Use initialiser to avoid default construction & assignment */
    CTSLink(const T &data) : m_Data(data) {}
};


/*** Definition of the base list class ***/

/* This class must not be used directly !
 * Use derived classes CSList and CTSList instead
 *
 * Implementation uses a circular list
 */

class CSList_Base {
private:

    /* Pointer to last element in the (circular) list */
    CISLink *m_Last;

    /*** Methods ***/

public:
    void Add(CISLink *link);
    void Append(CISLink *link);
    CISLink *Remove();
    void Clear() { m_Last = nullptr; }
    CSList_Base() { m_Last = nullptr; }
    friend class CSList_Base_Iter;
};

/* Implementation of base functions */
inline void
CSList_Base::Add(CISLink *data) {
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
CSList_Base::Append(CISLink *data) {
    if ( m_Last != nullptr ) {
        data->m_Next = m_Last->m_Next;
        m_Last = m_Last->m_Next = data;
    } else {
        m_Last = data->m_Next = data;
    }
}

/*** Definition & Implementation of an intrusive list ***/
template<class T>
class CISList : private CSList_Base {
public:
    void Add(T *data) { CSList_Base::Add(data); }
    void Clear() { CSList_Base::Clear(); }
    friend class CISList_Iter<T>;
};

/*** Definition & Implementation of a non-intrusive list ***/
template<class T>
class CTSList : protected CSList_Base {
public:
    void Add(const T &data);
    void Append(const T &data);
    void RemoveAll();
    void Clear() { CSList_Base::Clear(); }
    friend class CTSList_Iter<T>;
};

template<class T>
inline void CTSList<T>::Add(const T &data) {
    CSList_Base::Add(new CTSLink<T>(data));
}

template<class T>
inline void CTSList<T>::Append(const T &data) {
    CSList_Base::Append(new CTSLink<T>(data));
}

template<class T>
inline void CTSList<T>::RemoveAll() {
    CTSLink<T> *link = (CTSLink<T> *) CSList_Base::Remove();

    while ( link != nullptr ) {
        delete link;
        link = (CTSLink<T> *) CSList_Base::Remove();
    }
}

/*** Iterators for the different list classes ***/

/* For the base class - don't use directly */
class CSList_Base_Iter {
private:
    CISLink *m_CurrentElement;
    CSList_Base *m_CurrentList;
public:
    inline CSList_Base_Iter(CSList_Base &list);
    inline CISLink *Next();
    inline void Init(CSList_Base &list);
};

inline void CSList_Base_Iter::Init(CSList_Base &list) {
    m_CurrentList = &list;
    m_CurrentElement = m_CurrentList->m_Last;
}

inline CSList_Base_Iter::CSList_Base_Iter(CSList_Base &list) {
    Init(list);
}

inline CISLink *CSList_Base_Iter::Next() {
    CISLink *ret = (m_CurrentElement ?
                    (m_CurrentElement = m_CurrentElement->m_Next) :
                    (CISLink *) nullptr);
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
    void Init(CISList<T> &list) { CSList_Base_Iter::Init(list); }
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
    return (link ? &link->m_Data : (T *) nullptr);
}

template<class T>
inline void CTSList_Iter<T>::Init(CTSList<T> &list) {
    CSList_Base_Iter::Init(list);
}

#endif
