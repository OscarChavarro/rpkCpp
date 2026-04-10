#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"

LightListIterator::LightListIterator(LightList &list):
    iterator(list.entries())
{
}

Patch *
LightListIterator::First(LightList &list) {
    iterator.init(list.entries());

    LightInfo *li = iterator.nextOnSequence();
    if ( li != NULL ) {
        return li->light;
    } else {
        return NULL;
    }
}

Patch *
LightListIterator::Next() {
    LightInfo *li = iterator.nextOnSequence();
    if ( li ) {
        return li->light;
    } else {
        return NULL;
    }
}

LightList::LightList(const ArrayList<Patch *> *list, bool includeVirtualPatches) {
    LightInfo info = LightInfo();
    ColorRgb lightColor;

    totalFlux = 0.0;
    lightCount = 0;
    includeVirtual = includeVirtualPatches;
    totalImp = 0.0;

    for ( int i = 0; list != NULL && i < list->size(); i++ ) {
        Patch *light = list->get(i);
        if ( (!light->hasZeroVertices() || includeVirtual)
            && ( light->material->getEdf() != NULL ) ) {
            info.light = light;

            // calc emittedFlux
            if ( light->hasZeroVertices() ) {
                ColorRgb e;
                if ( light->material->getEdf() == NULL ) {
                    e.clear();
                } else {
                    e = light->material->getEdf()->phongEmittance(NULL, DIFFUSE_COMPONENT);
                }
                info.emittedFlux = e.average();
            } else {
                lightColor = PatchVisitor::averageEmittance(light, DIFFUSE_COMPONENT);
                info.emittedFlux = lightColor.average() * light->area;
            }

            totalFlux += info.emittedFlux;
            lightCount++;
            append(info);
        }
    }
}

LightList::~LightList() {
    removeAll();
}

CircularList<LightInfo> &
LightList::entries() {
    return *this;
}

/**
Returns sampled patch, scales x_1 back to a random in 0..1
*/
Patch *
LightList::sample(double *x1, double *pdf) {
    LightInfo *info;
    LightInfo *lastInfo;
    CircularListIterator<LightInfo> iterator(*this);

    double rnd = *x1 * totalFlux;
    double currentSum;

    info = iterator.nextOnSequence();
    while ( (info != NULL) && (info->light->hasZeroVertices()) && (!includeVirtual) ) {
        info = iterator.nextOnSequence();
    }

    if ( info == NULL ) {
        Error::warning("CLightList::sample", "No lights available");
        return NULL;
    }

    currentSum = info->emittedFlux;

    while ( (rnd > currentSum) && (info != NULL) ) {
        lastInfo = info;
        info = iterator.nextOnSequence();
        while ((info != NULL) && (info->light->hasZeroVertices()) && (!includeVirtual) ) {
            info = iterator.nextOnSequence();
        }

        if ( info != NULL ) {
            currentSum += info->emittedFlux;
        } else {
            info = lastInfo;
            rnd = currentSum - 1.0; // :-(  Damn float inaccuracies
        }
    }

    if ( info != NULL ) {
        *x1 = ((*x1 - ((currentSum - info->emittedFlux) / totalFlux)) /
               (info->emittedFlux / totalFlux));
        *pdf = info->emittedFlux / totalFlux;
        return (info->light);
    }

    return NULL;
}

double
LightList::evalPdfVirtual(const Patch *light, const Vector3D */*point*/) const {
    // EvalPDF for virtual patches (see EvalPDF)
    double probabilityDensityFunction;

    // Prob for choosing this light
    char all = DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT;

    ColorRgb e;
    if ( light->material->getEdf() == NULL) {
        e.clear();
    } else {
        e = light->material->getEdf()->phongEmittance(NULL, all);
    }
    probabilityDensityFunction = e.average() / totalFlux;

    return probabilityDensityFunction;
}

double
LightList::evalPdfReal(Patch *light, const Vector3D */*point*/) const {
    // Eval PDF for normal patches (see EvalPDF)
    ColorRgb color;
    double pdf;

    color = PatchVisitor::averageEmittance(light, DIFFUSE_COMPONENT);

    // Prob for choosing this light
    pdf = color.average() * light->area / totalFlux;

    return pdf;
}

double
LightList::evalPdf(Patch *light, const Vector3D *point) const {
    // TODO!!!  1) patch should become class
    //          2) virtual patch should become child-class
    //          3) this method should be handled by specialisation
    if ( totalFlux < Numeric::EPSILON ) {
        return 0.0;
    }
    if ( light->hasZeroVertices() ) {
        return evalPdfVirtual(light, point);
    } else {
        return evalPdfReal(light, point);
    }
}

/* Important light sampling */

double
LightList::cmptOneLightImpVrtl(
    const Patch *light,
    const Vector3D *,
    const Vector3D *,
    float) {
    // ComputeOneLightImportance for virtual patches
    char all = DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT;

    ColorRgb e;

    if ( light->material->getEdf() == NULL ) {
        e.clear();
    } else {
        e = light->material->getEdf()->phongEmittance(NULL, all);
    }
    return e.average();
}

double
LightList::computeOneLightImportanceReal(
    const Patch *light,
    const Vector3D *point,
    const Vector3D *normal,
    float emittedFlux)
{
    // ComputeOneLightImportance for real patches
    int tried = 0;  // No positions on the patch are tried yet
    int done = false;
    double contribution = 0.0;
    Vector3D lightPoint;
    Vector3D lightNormal;
    Vector3D dir;
    double cosRayPatch;
    double cosRayLight;
    double dist2;

    while ( !done && tried <= light->numberOfVertices ) {
        // Choose a point on the patch according to 'tried'

        if ( tried == 0 ) {
            lightPoint = light->midPoint;
            lightNormal = light->normal;
        } else {
            lightPoint = *(light->vertex[tried - 1]->point);
            if ( light->vertex[tried - 1]->normal != NULL ) {
                lightNormal = *(light->vertex[tried - 1]->normal);
            } else {
                lightNormal = light->normal;
            }
        }

        // Estimate the contribution

        // Ray direction (but no ray is shot of course)
        Vector3D copy(point->x, point->y, point->z);

        dir.subtraction(lightPoint, copy);
        dist2 = dir.norm2();

        // Cosines have an addition distance length in them
        cosRayLight = -dir.dotProduct(lightNormal);
        cosRayPatch = dir.dotProduct(*normal);

        if ( cosRayLight > 0 && cosRayPatch > 0 ) {
            // Orientation of surfaces ok.
            // BRDF is not taken into account since
            // we're expecting diffuse/glossy surfaces here.

            contribution = (cosRayPatch * cosRayLight * emittedFlux / (M_PI * dist2));
            done = true;
        }

        tried++; // Try next point on light
    }

    return contribution;
}

double
LightList::computeOneLightImportance(
    const Patch *light,
    const Vector3D *point,
    const Vector3D *normal,
    float emittedFlux)
{
    // TODO!!!  1) patch should become class
    //          2) virtual patch should become child-class
    //          3) this method should be handled by specialisation
    if ( light->hasZeroVertices() ) {
        return cmptOneLightImpVrtl(light, point, normal, emittedFlux);
    } else {
        return computeOneLightImportanceReal(light, point, normal, emittedFlux);
    }
}

void
LightList::computeLightImportance(const Vector3D *point, const Vector3D *normal) {
    if ((point->equals(lastPoint, Numeric::EPSILON_FLOAT)) &&
        (normal->equals(lastNormal, Numeric::EPSILON_FLOAT)) ) {
        return; // Still ok !!
    }


    LightInfo *info;
    CircularListIterator<LightInfo> iterator(*this);
    double imp;

    totalImp = 0.0;

    // next
    info = iterator.nextOnSequence();
    while ((info != NULL) && (info->light->hasZeroVertices()) && (!includeVirtual) ) {
        info = iterator.nextOnSequence();
    }

    while ( info ) {
        imp = computeOneLightImportance(info->light, point, normal,
                                        info->emittedFlux);
        totalImp += ((float)(imp));
        info->importance = ((float)(imp));

        // next
        info = iterator.nextOnSequence();
        while ( (info != NULL) && (info->light->hasZeroVertices()) && (!includeVirtual) ) {
            info = iterator.nextOnSequence();
        }
    }
}

Patch *
LightList::sampleImportant(const Vector3D *point, const Vector3D *normal, double *x1, double *pdf) {
    const LightInfo *info;
    const LightInfo *lastInfo;
    CircularListIterator<LightInfo> iterator(*this);
    double rnd;
    double currentSum;

    computeLightImportance(point, normal);

    if ( totalImp == 0 ) {
        // No light is important, but we must return one (->optimize ?)
        return (sample(x1, pdf));
    }

    rnd = *x1 * totalImp;

    // Next
    info = iterator.nextOnSequence();
    while ( (info != NULL) && (info->light->hasZeroVertices()) && (!includeVirtual) ) {
        info = iterator.nextOnSequence();
    }

    if ( info == NULL ) {
        Error::warning("CLightList::sample", "No lights available");
        return NULL;
    }

    currentSum = info->importance;

    while ( (rnd > currentSum) && (info != NULL) ) {
        lastInfo = info;

        // next
        info = iterator.nextOnSequence();
        while ( info != NULL && info->light->hasZeroVertices() && !includeVirtual ) {
            info = iterator.nextOnSequence();
        }

        if ( info != NULL ) {
            currentSum += info->importance;
        } else {
            info = lastInfo;
            rnd = currentSum - 1.0; // :-(  Damn float inaccuracies
        }
    }

    if ( info != NULL ) {
        *x1 = ((*x1 - ((currentSum - info->importance) / totalImp)) /
               (info->importance / totalImp));
        *pdf = info->importance / totalImp;
        return (info->light);
    }

    return NULL;
}

double
LightList::evalPdfImportant(
    const Patch *light,
    const Vector3D */*lightPoint*/,
    const Vector3D *litPoint,
    const Vector3D *normal)
{
    double pdf;
    const LightInfo *info;
    CircularListIterator<LightInfo> iterator(*this);

    computeLightImportance(litPoint, normal);

    // Search the light in the list :-(

    while ( (info = iterator.nextOnSequence()) && info->light != light );

    if ( info == NULL ) {
        Error::warning("CLightList::evalPdfImportant", "Could not find light");
        return 0.0;
    }

    // Prob for choosing this light
    if ( totalImp < Numeric::EPSILON ) {
        pdf = 0.0;
    } else {
        pdf = info->importance / totalImp;
    }

    return pdf;
}
