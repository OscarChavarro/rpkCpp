#ifndef __MGF_CONTEXT__
#define __MGF_CONTEXT__

#include "skin/RadianceMethod.h"

// Entities
#define MGF_ENTITY_COLOR 1 // c
#define MGF_ENTITY_CCT 2 // cct
#define MGF_ENTITY_CONE 3 // cone
#define MGF_ENTITY_C_MIX 4 // cMix
#define MGF_ENTITY_C_SPEC 5 // cSpec
#define MGF_ENTITY_CXY 6 // cxy
#define MGF_ENTITY_CYLINDER 7 // cyl
#define MGF_ENTITY_ED 8 // ed
#define MGF_ENTITY_FACE 9 // f
#define MG_E_INCLUDE 10 // i
#define MG_E_IES 11 // ies
#define MGF_ENTITY_IR 12 // ir
#define MGF_ENTITY_MATERIAL 13 // m
#define MGF_ENTITY_NORMAL 14 // n
#define MGF_ENTITY_OBJECT 15 // o
#define MGF_ENTITY_POINT 16 // p
#define MGF_ENTITY_PRISM 17 // prism
#define MGF_ENTITY_RD 18 // rd
#define MGF_ENTITY_RING 19 // ring
#define MGF_ENTITY_RS 20 // rs
#define MGF_ENTITY_SIDES 21 // sides
#define MGF_ENTITY_SPHERE 22 // sph
#define MGF_ENTITY_TD 23 // td
#define MGF_ENTITY_TORUS 24 // torus
#define MGF_ENTITY_TS 25 // ts
#define MGF_ENTITY_VERTEX 26 // v
#define MGF_ENTITY_XF 27 // xf
#define MGF_ENTITY_FACE_WITH_HOLES 28 // fh (version 2 MGF)
#define MGF_TOTAL_NUMBER_OF_ENTITIES 29

#define MGF_MAXIMUM_ENTITY_NAME_LENGTH    6

class MgfContext {
  public:
    // Parameters received from main program
    RadianceMethod *radianceMethod;
    bool singleSided;
    char *currentVertexName;
    int numberOfQuarterCircleDivisions;

    // Global variables on the MGF reader context
    char entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];

    // Return model
    MgfContext();
};

#endif
