#ifndef __MGF_GEOMETRY__
#define __MGF_GEOMETRY__

#include "common/linealAlgebra/Vector3Dd.h"
#include "io/context/ParseSession.h"

class MgfGeometry {
  public:
    static int mgfEntitySphere(int ac, const char **av, ParseSession *context);
    static int mgfEntityTorus(int ac, const char **av, ParseSession *context);
    static int mgfEntityCylinder(int ac, const char **av, ParseSession *context);
    static int mgfEntityRing(int ac, const char **av, ParseSession *context);
    static int mgfEntityCone(int ac, const char **av, ParseSession *context);
    static int mgfEntityPrism(int ac, const char **av, ParseSession *context);
    static int mgfEntityFaceWithHoles(int ac, const char **av, ParseSession *context);

  private:
    static constexpr int MGF_PV_SIZE = 24;
    static constexpr char FLOAT_FORMAT[] = "%.12g";

    static void mgfMakeAxes(Vector3Dd *u, Vector3Dd *v, const Vector3Dd *w, double epsilon);
};

#endif
