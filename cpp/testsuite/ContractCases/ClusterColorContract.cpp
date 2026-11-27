#include <cassert>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/processing/ClusterTraversalStrategy.h"
#include "vsdk/toolkit/galerkin/processing/visitors/MaximumRadianceVisitor.h"
#include "vsdk/toolkit/galerkin/processing/visitors/PowerAccumulatorVisitor.h"
static void check(const ColorRgb &c, double r, double g, double b) {
    assert(c.getR() == r && c.getG() == g && c.getB() == b);
}
int main() {
    Vector3Dd p1(0,0,0), p2(1,0,0), p3(0,1,0);
    RpkVertex v1(&p1, nullptr, nullptr, new java::ArrayList<Patch *>());
    RpkVertex v2(&p2, nullptr, nullptr, new java::ArrayList<Patch *>());
    RpkVertex v3(&p3, nullptr, nullptr, new java::ArrayList<Patch *>());
    Patch patch(3, &v1, &v2, &v3, nullptr);
    GalerkinState state;
    state.galerkinIterationMethod = GalerkinIterationMethod::JACOBI;
    GalerkinElement element(static_cast<RpkGeometry *>(nullptr), &state);
    element.patch = &patch;
    element.area = 2.0;
    element.radiance[0] = ColorRgb(2, -1, 4);
    check(GalerkinBasis::radianceAtPoint(&element, element.radiance, 0.25, 0.5), 2, -1, 4);
    check(GalerkinBasis::radianceAtPoint(&element, nullptr, 0.25, 0.5), 0, 0, 0);
    MaximumRadianceVisitor maximum;
    PowerAccumulatorVisitor power(ColorRgb(1, 2, 3), patch.midPoint().add(patch.getNormal()));
    maximum.visit(&element, &state);
    power.visit(&element, &state);
    check(maximum.getAccumulatedRadiance(), 2, 0, 4);
    check(power.getAccumulatedRadiance(), 5, 0, 11);
    element.radiance[0] = ColorRgb(-3, 5, 1);
    maximum.visit(&element, &state);
    power.visit(&element, &state);
    check(maximum.getAccumulatedRadiance(), 2, 5, 4);
    check(power.getAccumulatedRadiance(), -5, 12, 5);
    GalerkinElement root(static_cast<RpkGeometry *>(nullptr), &state);
    root.area = 2.0;
    root.irregularSubElements = new java::ArrayList<Element *>();
    root.irregularSubElements->add(&element);
    element.flags &= static_cast<unsigned char>(~IS_CLUSTER_MASK);
    state.clusteringStrategy = GalerkinClusteringStrategy::ORIENTED;
    check(ClusterTraversalStrategy::clusterRadianceToSamplePoint(&root, patch.midPoint().add(patch.getNormal()), nullptr, &state), -12, 20, 4);
    delete root.irregularSubElements;
    root.irregularSubElements = nullptr;
    return 0;
}
