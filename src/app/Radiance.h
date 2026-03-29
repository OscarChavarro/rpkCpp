#ifndef __RADIANCE__
#define __RADIANCE__

#include "scene/RadianceMethod.h"

class Radiance final {
  public:
    static void radianceParseOptions(int *argc, char **argv, RadianceMethod **newRadianceMethod);
    static void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene);

  private:
    static void selectRadianceMethod(const int *argc, char **argv, RadianceMethod **newRadianceMethod);
};

#endif
