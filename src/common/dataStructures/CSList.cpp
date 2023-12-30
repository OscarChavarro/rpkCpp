#include "common/dataStructures/CSList.h"

CISLink *CSList_Base::Remove() {
    // Remove first element and return it

    if ( m_Last == nullptr ) {
        // Not really an error.
        // Error("CList_Base::Remove", "Remove from empty list");
        return nullptr;
    }

    CISLink *first = m_Last->m_Next;

    if ( first == m_Last ) {
        m_Last = nullptr;
    } else {
        m_Last->m_Next = first->m_Next;
    }

    return first;
}
