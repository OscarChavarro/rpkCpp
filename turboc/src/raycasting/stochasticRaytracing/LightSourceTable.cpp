#include "raycasting/stochasticRaytracing/LightSourceTable.h"

LightSourceTable::LightSourceTable():
    patch(NULL),
    flux(0.0)
{
}

LightSourceTable::LightSourceTable(Patch *patch, double flux):
    patch(patch),
    flux(flux)
{
}
