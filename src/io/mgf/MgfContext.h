#ifndef __MGF_CONTEXT__
#define __MGF_CONTEXT__

#include "skin/RadianceMethod.h"
#include "io/mgf/MgfReaderContext.h"

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

// Error codes
#define MGF_OK 0 // normal return value
#define MGF_ERROR_UNKNOWN_ENTITY 1
#define MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS 2
#define MGF_ERROR_ARGUMENT_TYPE 3
#define MGF_ERROR_ILLEGAL_ARGUMENT_VALUE 4
#define MGF_ERROR_UNDEFINED_REFERENCE 5
#define MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE 6
#define MGF_ERROR_IN_INCLUDED_FILE 7
#define MGF_ERROR_OUT_OF_MEMORY 8
#define MGF_ERROR_FILE_SEEK_ERROR 9
#define MGF_ERROR_LINE_TOO_LONG 11
#define MGF_ERROR_UNMATCHED_CONTEXT_CLOSE 12
#define MGF_NUMBER_OF_ERRORS 13

// Objects 'o' contexts can be nested this deep
#define MAXIMUM_GEOMETRY_STACK_DEPTH 100

class MgfTransformContext;
class MgfColorContext;
class LookUpTable;

class MgfContext {
  public:
    // Parameters received from main program
    RadianceMethod *radianceMethod;
    bool singleSided;
    char *currentVertexName;
    int numberOfQuarterCircleDivisions;
    bool monochrome;

    // Internal variables on the MGF reader context
    char entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    const char *errorCodeMessages[MGF_NUMBER_OF_ERRORS];
    MgfReaderContext *readerContext;
    int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext *context);
    int (*supportCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext *context);
    Material *currentMaterial;
    char *currentMaterialName;
    java::ArrayList<Geometry *> **geometryStackPtr;
    java::ArrayList<Geometry *> *geometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];
    java::ArrayList<Vector3D *> *currentPointList;
    java::ArrayList<Vector3D *> *currentNormalList;
    java::ArrayList<Vertex *> *currentVertexList;
    java::ArrayList<Patch *> *currentFaceList;
    java::ArrayList<Geometry *> *currentGeometryList;
    MgfTransformContext *transformContext;
    MgfColorContext *unNamedColorContext;
    MgfColorContext *currentColor;
    bool inSurface;
    bool inComplex;
    LookUpTable *vertexLookUpTable;

    // Return model

    MgfContext();
    ~MgfContext();
};

#include "io/mgf/MgfTransformContext.h"
#include "io/mgf/MgfColorContext.h"
#include "io/mgf/lookup.h"

#endif
