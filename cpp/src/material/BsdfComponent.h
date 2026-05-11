#ifndef BSDF_COMPONENT__
#define BSDF_COMPONENT__

enum BsdfComponent {
    BRDF_DIFFUSE_COMPONENT = 0x01,
    BRDF_GLOSSY_COMPONENT = 0x02,
    BRDF_SPECULAR_COMPONENT = 0x04,
    BTDF_DIFFUSE_COMPONENT = 0x08,
    BTDF_GLOSSY_COMPONENT = 0x10,
    BTDF_SPECULAR_COMPONENT = 0x20
};

class BsdfComponentInfo final {
  public:
    static constexpr int BSDF_DIFFUSE_COMPONENT = BTDF_DIFFUSE_COMPONENT | BRDF_DIFFUSE_COMPONENT;
    static constexpr int BSDF_GLOSSY_COMPONENT = BTDF_GLOSSY_COMPONENT | BRDF_GLOSSY_COMPONENT;
    static constexpr int BSDF_SPECULAR_COMPONENT = BTDF_SPECULAR_COMPONENT | BRDF_SPECULAR_COMPONENT;
    static constexpr int BSDF_COMPONENTS = 6;
    static constexpr int BSDF_ALL_COMPONENTS = BRDF_DIFFUSE_COMPONENT
                                               | BRDF_GLOSSY_COMPONENT
                                               | BRDF_SPECULAR_COMPONENT
                                               | BTDF_DIFFUSE_COMPONENT
                                               | BTDF_GLOSSY_COMPONENT
                                               | BTDF_SPECULAR_COMPONENT;
};

#endif
