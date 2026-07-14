#include "vsdk/toolkit/raycasting/stochasticRaytracing/LightSourceTable.h"

LightSourceTable::LightSourceTable():
    patch(nullptr),
    flux(0.0)
{
}

LightSourceTable::LightSourceTable(Patch *patch, double flux):
    patch(patch),
    flux(flux)
{
}
