#ifndef __MGF_HANDLER_TRANSFORM__
#define __MGF_HANDLER_TRANSFORM__

#include "common/linealAlgebra/Vector3Dd.h"
#include "io/context/ParseSession.h"

class Matrix4x4d;
class TransformArray;
class TransformContext;
class TransformStackContext;

/**
The transformation handler should do most of the work that needs
doing. Just pass it any xf entities, then use the associated
functions to transform and translate positions, transform vectors
(without translation), rotate vectors (without scaling) and scale
values appropriately.

The routines mgfTransformPoint and mgfTransformVector takes two
3-D vectors (which may be identical), transforms the second and
puts the result into the first.
*/

class MgfHandlerTransform {
  public:
    static int handleTransformationEntity(int ac, const char **av, ParseSession *context);
    static void mgfTransformPoint(Vector3Dd *v1, const Vector3Dd *v2, const ParseSession *context); // Transform point
    static void mgfTransformVector(Vector3Dd *v1, const Vector3Dd *v2, const ParseSession *context); // Transform vector
    static void mgfTransformFreeMemory(ParseSession *context);

  private:
    static long computeUniqueId(const Matrix4x4d *xfm);
    static double d2r(double a);
    static int checkForBadArguments(int ac, char **av, const char *fl);
    static bool checkArgument(int a, const char *l, int ac, char **av, int i);
    static int transformName(const TransformArray *ap, ParseSession *context);
    static TransformStackContext *newTransform(int ac, const char **av, ParseSession *context);
    static void finish(int count, TransformContext *ret, const Matrix4x4d *transformMatrix, double scaTransform);
    static int xf(TransformContext *ret, int ac, char **av);
    static bool compactTransformArguments(ParseSession *context, const TransformStackContext *stackContext);
};

#endif
