#ifndef __MGF_ENTITY__
#define __MGF_ENTITY__

// Entities
enum MgfEntity {
    COLOR = 1, // c
    CCT = 2, // cct
    CONE = 3, // cone
    C_MIX = 4, // cMix
    C_SPEC = 5, // cSpec
    CXY = 6, // cxy
    CYLINDER = 7, // cyl
    ED = 8, // ed
    FACE = 9, // f
    INCLUDE = 10, // i
    IES = 11, // ies
    IR = 12, // ir
    MGF_MATERIAL = 13, // m
    MGF_NORMAL = 14, // n
    OBJECT = 15, // o
    MGF_POINT = 16, // p
    PRISM = 17, // prism
    RD = 18, // rd
    RING = 19, // ring
    RS = 20, // rs
    SIDES = 21, // sides
    SPHERE = 22, // sph
    TD = 23, // td
    TORUS = 24, // torus
    TS = 25, // ts
    VERTEX = 26, // v
    TRANSFORM = 27, // xf
    FACE_WITH_HOLES = 28, // fh (version 2 MGF)
    TOTAL_NUMBER_OF_ENTITIES = 29
};

#define MGF_MAXIMUM_ENTITY_NAME_LENGTH    6

#endif
