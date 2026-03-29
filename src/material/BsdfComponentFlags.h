#ifndef __BSDF_COMPONENT_FLAGS__
#define __BSDF_COMPONENT_FLAGS__

enum XxdfComponentFlag {
    DIFFUSE_COMPONENT = 1,
    GLOSSY_COMPONENT = 2,
    SPECULAR_COMPONENT = 4
};

constexpr int XXDF_COMPONENTS = 3;
constexpr int NO_COMPONENTS = 0;
constexpr int ALL_COMPONENTS = DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT;

enum BsdfComponent {
    BRDF_DIFFUSE_COMPONENT = 0x01,
    BRDF_GLOSSY_COMPONENT = 0x02,
    BRDF_SPECULAR_COMPONENT = 0x04,
    BTDF_DIFFUSE_COMPONENT = 0x08,
    BTDF_GLOSSY_COMPONENT = 0x10,
    BTDF_SPECULAR_COMPONENT = 0x20
};

constexpr int BSDF_DIFFUSE_COMPONENT = BTDF_DIFFUSE_COMPONENT | BRDF_DIFFUSE_COMPONENT;
constexpr int BSDF_GLOSSY_COMPONENT = BTDF_GLOSSY_COMPONENT | BRDF_GLOSSY_COMPONENT;
constexpr int BSDF_SPECULAR_COMPONENT = BTDF_SPECULAR_COMPONENT | BRDF_SPECULAR_COMPONENT;
constexpr int BSDF_COMPONENTS = 6;
constexpr int BSDF_ALL_COMPONENTS = BRDF_DIFFUSE_COMPONENT
                                           | BRDF_GLOSSY_COMPONENT
                                           | BRDF_SPECULAR_COMPONENT
                                           | BTDF_DIFFUSE_COMPONENT
                                           | BTDF_GLOSSY_COMPONENT
                                           | BTDF_SPECULAR_COMPONENT;

class BsdfComponentFlag {
  public:
    static constexpr int
    bsdfIndexToComp(const int index) {
        return 1 << index;
    }

    static constexpr int
    getBrdfFlags(const int bsflags) {
        return bsflags & ALL_COMPONENTS;
    }

    static constexpr int
    getBtdfFlags(const int bsflags) {
        return (bsflags >> XXDF_COMPONENTS) & ALL_COMPONENTS;
    }
};

#endif
