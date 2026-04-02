#ifndef __XXDF_COMPONENT_FLAG__
#define __XXDF_COMPONENT_FLAG__

enum XxdfComponentFlag {
    DIFFUSE_COMPONENT = 1,
    GLOSSY_COMPONENT = 2,
    SPECULAR_COMPONENT = 4
};

class XxdfComponentFlagInfo final {
  public:
    static constexpr int XXDF_COMPONENTS = 3;
    static constexpr int NO_COMPONENTS = 0;
    static constexpr int ALL_COMPONENTS = DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT;
};

#endif
