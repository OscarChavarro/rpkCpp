/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef __XXDF__
#define __XXDF__

#include "common/linealAlgebra/Vector3D.h"

/**
Contributions to outgoing radiance are divided into three components :
*/

#define DIFFUSE_COMPONENT 1
#define GLOSSY_COMPONENT 2
#define SPECULAR_COMPONENT 4

// Convert index to component value

// There are three components
#define XXDF_COMPONENTS 3

#define NO_COMPONENTS 0
#define ALL_COMPONENTS (DIFFUSE_COMPONENT|GLOSSY_COMPONENT|SPECULAR_COMPONENT)

// Bsdf's have 6 components : 3 for reflection, 3 for transmission

#define BRDF_DIFFUSE_COMPONENT 0x01
#define BRDF_GLOSSY_COMPONENT 0x02
#define BRDF_SPECULAR_COMPONENT 0x04
#define BTDF_DIFFUSE_COMPONENT 0x08
#define BTDF_GLOSSY_COMPONENT 0x10
#define BTDF_SPECULAR_COMPONENT 0x20

#define BSDF_DIFFUSE_COMPONENT (BTDF_DIFFUSE_COMPONENT | BRDF_DIFFUSE_COMPONENT)
#define BSDF_GLOSSY_COMPONENT (BTDF_GLOSSY_COMPONENT | BRDF_GLOSSY_COMPONENT)
#define BSDF_SPECULAR_COMPONENT (BTDF_SPECULAR_COMPONENT | BRDF_SPECULAR_COMPONENT)

// Convert index to component value
#define BSDF_INDEX_TO_COMP(i) (1 << (i))

// There are six components
#define BSDF_COMPONENTS 6

#define NO_COMPONENTS 0
#define BSDF_ALL_COMPONENTS (BRDF_DIFFUSE_COMPONENT|BRDF_GLOSSY_COMPONENT|BRDF_SPECULAR_COMPONENT|BTDF_DIFFUSE_COMPONENT|BTDF_GLOSSY_COMPONENT|BTDF_SPECULAR_COMPONENT)

// Converts BSDFFLAGS to XXDFFLAGS
#define GET_BRDF_FLAGS(bsflags) ((bsflags) & ALL_COMPONENTS)
#define GET_BTDF_FLAGS(bsflags) (((bsflags) >> XXDF_COMPONENTS) & ALL_COMPONENTS)

// Converts XXDFFLAGS to BSDFFLAGS
#define SET_BRDF_FLAGS(xxflags) ((xxflags) & ALL_COMPONENTS)
#define SET_BTDF_FLAGS(xxflags) (((xxflags) & ALL_COMPONENTS) << XXDF_COMPONENTS)

typedef char BSDF_FLAGS;

class RefractionIndex {
  public:
    float nr;
    float ni;
};

extern float complexToGeometricRefractionIndex(RefractionIndex nc);
extern Vector3D idealReflectedDirection(const Vector3D *in, const Vector3D *normal);

extern Vector3D
idealRefractedDirection(
    const Vector3D *in,
    const Vector3D *normal,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    int *totalInternalReflection);

#endif
