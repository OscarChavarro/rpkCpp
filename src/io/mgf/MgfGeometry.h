#ifndef __MGF_GEOMETRY__
#define __MGF_GEOMETRY__

#include "common/linealAlgebra/Vector3Dd.h"
#include "io/context/MgfParseSession.h"

class MgfGeometry {
  public:
    static int mgfEntitySphere(int ac, const char **av, MgfParseSession *context);
    static int mgfEntityTorus(int ac, const char **av, MgfParseSession *context);
    static int mgfEntityCylinder(int ac, const char **av, MgfParseSession *context);
    static int mgfEntityRing(int ac, const char **av, MgfParseSession *context);
    static int mgfEntityCone(int ac, const char **av, MgfParseSession *context);
    static int mgfEntityPrism(int ac, const char **av, MgfParseSession *context);
    static int mgfEntityFaceWithHoles(int ac, const char **av, MgfParseSession *context);

  private:
    static void mgfMakeAxes(Vector3Dd *u, Vector3Dd *v, const Vector3Dd *w, double epsilon);
};

#endif
