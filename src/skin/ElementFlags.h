#ifndef __ELEMENT_FLAGS__
#define __ELEMENT_FLAGS__

enum ElementFlags {
    // If set, indicates that the element is a cluster element. If not set, the element is a surface element
    IS_CLUSTER_MASK = 0x01,
    // If the element is or contains surfaces emitting light spontaneously
    IS_LIGHT_SOURCE_MASK = 0x02,
    // Set when all interactions have been created for a toplevel element
    INTERACTIONS_CREATED_MASK = 0x04
};

#endif
