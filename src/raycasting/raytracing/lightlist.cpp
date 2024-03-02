#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "raycasting/raytracing/lightlist.h"

CLightList *GLOBAL_lightList = nullptr;

CLightList::CLightList(java::ArrayList<Patch *> *list, bool includeVirtualPatches) {
    CLightInfo info{};
    COLOR lightColor;

    totalFlux = 0.0;
    lightCount = 0;
    includeVirtual = includeVirtualPatches;
    totalImp = 0.0;

    for ( int i = 0; list != nullptr && i < list->size(); i++ ) {
        Patch *light = list->get(i);
        if ( !light->hasZeroVertices() || includeVirtual ) {
            if ( light->surface->material->edf != nullptr ) {
                info.light = light;

                // calc emittedFlux
                if ( light->hasZeroVertices()) {
                    COLOR e = edfEmittance(light->surface->material->edf, nullptr,
                                           DIFFUSE_COMPONENT);
                    info.emittedFlux = colorAverage(e);
                } else {
                    lightColor = light->averageEmittance(DIFFUSE_COMPONENT);
                    info.emittedFlux = colorAverage(lightColor) * light->area;
                }

                totalFlux += info.emittedFlux;
                lightCount++;
                Append(info);
            }
        }
    }
}

CLightList::~CLightList() {
    RemoveAll();
}

/**
Returns sampled patch, scales x_1 back to a random in 0..1
*/
Patch *
CLightList::sample(double *x1, double *pdf) {
    CLightInfo *info, *lastInfo;
    CTSList_Iter<CLightInfo> iterator(*this);

    double rnd = *x1 * totalFlux;
    double currentSum;

    info = iterator.Next();
    while ( (info != nullptr) && (info->light->hasZeroVertices()) && (!includeVirtual) ) {
        info = iterator.Next();
    }

    if ( info == nullptr ) {
        logWarning("CLightList::sample", "No lights available");
        return nullptr;
    }

    currentSum = info->emittedFlux;

    while ( (rnd > currentSum) && (info != nullptr) ) {
        lastInfo = info;
        info = iterator.Next();
        while ((info != nullptr) && (info->light->hasZeroVertices()) && (!includeVirtual)) {
            info = iterator.Next();
        }

        if ( info != nullptr ) {
            currentSum += info->emittedFlux;
        } else {
            info = lastInfo;
            rnd = currentSum - 1.0; // :-(  Damn float inaccuracies
        }
    }

    if ( info != nullptr ) {
        *x1 = ((*x1 - ((currentSum - info->emittedFlux) / totalFlux)) /
               (info->emittedFlux / totalFlux));
        *pdf = info->emittedFlux / totalFlux;
        return (info->light);
    }

    return nullptr;
}

double
CLightList::evalPdfVirtual(Patch *light, Vector3D */*point*/) const {
    // EvalPDF for virtual patches (see EvalPDF)
    double pdf;

    // Prob for choosing this light
    XXDFFLAGS all = DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT;

    COLOR e = edfEmittance(light->surface->material->edf, nullptr, all);
    pdf = colorAverage(e) / totalFlux;

    return pdf;
}

double
CLightList::evalPdfReal(Patch *light, Vector3D */*point*/) const {
    // Eval PDF for normal patches (see EvalPDF)
    COLOR col;
    double pdf;

    col = light->averageEmittance(DIFFUSE_COMPONENT);

    // Prob for choosing this light
    pdf = colorAverage(col) * light->area / totalFlux;

    return pdf;
}

double
CLightList::evalPdf(Patch *light, Vector3D *point) {
    // TODO!!!  1) patch should become class
    //          2) virtual patch should become child-class
    //          3) this method should be handled by specialisation
    if ( totalFlux < EPSILON ) {
        return 0.0;
    }
    if ( light->hasZeroVertices() ) {
        return evalPdfVirtual(light, point);
    } else {
        return evalPdfReal(light, point);
    }
}

/*************************************************************************/
/* Important light sampling */

double
CLightList::computeOneLightImportanceVirtual(Patch *light,
                                             const Vector3D *,
                                             const Vector3D *,
                                             float) {
    // ComputeOneLightImportance for virtual patches
    XXDFFLAGS all = DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT;

    COLOR e = edfEmittance(light->surface->material->edf, nullptr, all);
    return colorAverage(e);
}

double
CLightList::computeOneLightImportanceReal(Patch *light,
                                          const Vector3D *point,
                                          const Vector3D *normal,
                                          float emittedFlux) {
    // ComputeOneLightImportance for real patches
    int tried = 0;  // No positions on the patch are tried yet
    int done = false;
    double contribution = 0.0;
    Vector3D lightPoint, light_normal;
    Vector3D dir;
    double cosRayPatch, cosRayLight, dist2;

    while ( !done && tried <= light->numberOfVertices ) {
        // Choose a point on the patch according to 'tried'

        if ( tried == 0 ) {
            lightPoint = light->midpoint;
            light_normal = light->normal;
        } else {
            lightPoint = *(light->vertex[tried - 1]->point);
            if ( light->vertex[tried - 1]->normal != nullptr ) {
                light_normal = *(light->vertex[tried - 1]->normal);
            } else {
                light_normal = light->normal;
            }
        }

        // Previous
        //    case 0: u = v = 0.5; break;
        //    case 1: u = v = 0.0; break;
        //    case 2: u = 1.0; v = 0.0; break;
        //    case 3: u = 0.0; v = 1.0; break;
        //    case 4: u = v = 1.0; break;
        //
        //      // TODO ? allow more vertices
        //    }
        //
        //    patchPoint(light, u, v, &lightPoint);

        // Estimate the contribution

        // light_normal = PatchNormalAtUV(light, u, v);

        // Ray direction (but no ray is shot of course)
        Vector3D copy(point->x, point->y, point->z);

        vectorSubtract(lightPoint, copy, dir);
        dist2 = vectorNorm2(dir);

        // Check normals

        // Cosines have an addition dist length in them

        cosRayLight = -vectorDotProduct(dir, light_normal);
        cosRayPatch = vectorDotProduct(dir, *normal);

        if ( cosRayLight > 0 && cosRayPatch > 0 ) {
            // Orientation of surfaces ok.
            // BRDF is not taken into account since
            // we're expecting diffuse/glossy surfaces here.

            contribution = (cosRayPatch * cosRayLight * emittedFlux / (M_PI * dist2));
            done = true;
        }

        tried++; // trie next point on light
    }

    return contribution;
}

double
CLightList::computeOneLightImportance(Patch *light,
                                             const Vector3D *point,
                                             const Vector3D *normal,
                                             float emittedFlux) {
    // TODO!!!  1) patch should become class
    //          2) virtual patch should become child-class
    //          3) this method should be handled by specialisation
    if ( light->hasZeroVertices() ) {
        return computeOneLightImportanceVirtual(light, point, normal, emittedFlux);
    } else {
        return computeOneLightImportanceReal(light, point, normal, emittedFlux);
    }
}

void
CLightList::computeLightImportance(Vector3D *point, Vector3D *normal) {
    if ((vectorEqual(*point, lastPoint, EPSILON)) &&
        (vectorEqual(*normal, lastNormal, EPSILON))) {
        return; // Still ok !!
    }


    CLightInfo *info;
    CTSList_Iter<CLightInfo> iterator(*this);
    double imp;

    totalImp = 0.0;

    // next
    info = iterator.Next();
    while ((info != nullptr) && (info->light->hasZeroVertices()) && (!includeVirtual)) {
        info = iterator.Next();
    }

    while ( info ) {
        imp = computeOneLightImportance(info->light, point, normal,
                                        info->emittedFlux);
        totalImp += (float)imp;
        info->importance = (float)imp;

        // next
        info = iterator.Next();
        while ( (info != nullptr) && (info->light->hasZeroVertices()) && (!includeVirtual) ) {
            info = iterator.Next();
        }
    }
}

Patch *
CLightList::sampleImportant(Vector3D *point, Vector3D *normal, double *x1, double *pdf) {
    CLightInfo *info, *lastInfo;
    CTSList_Iter<CLightInfo> iterator(*this);
    double rnd;
    double currentSum;

    computeLightImportance(point, normal);

    if ( totalImp == 0 ) {
        // No light is important, but we must return one (->optimize ?)
        return (sample(x1, pdf));
    }

    rnd = *x1 * totalImp;

    // Next
    info = iterator.Next();
    while ((info != nullptr) && (info->light->hasZeroVertices()) && (!includeVirtual)) {
        info = iterator.Next();
    }

    if ( info == nullptr ) {
        logWarning("CLightList::sample", "No lights available");
        return nullptr;
    }

    currentSum = info->importance;

    while ( (rnd > currentSum) && (info != nullptr) ) {
        lastInfo = info;

        // next
        info = iterator.Next();
        while ( info != nullptr && info->light->hasZeroVertices() && !includeVirtual ) {
            info = iterator.Next();
        }

        if ( info != nullptr ) {
            currentSum += info->importance;
        } else {
            info = lastInfo;
            rnd = currentSum - 1.0; // :-(  Damn float inaccuracies
        }
    }

    if ( info != nullptr ) {
        *x1 = ((*x1 - ((currentSum - info->importance) / totalImp)) /
               (info->importance / totalImp));
        *pdf = info->importance / totalImp;
        return (info->light);
    }

    return nullptr;
}

double
CLightList::evalPdfImportant(Patch *light, Vector3D */*lightPoint*/,
                             Vector3D *litPoint, Vector3D *normal) {
    double pdf;
    CLightInfo *info;
    CTSList_Iter<CLightInfo> iterator(*this);

    computeLightImportance(litPoint, normal);

    // Search the light in the list :-(

    while ( (info = iterator.Next()) && info->light != light ) {
    }

    if ( info == nullptr ) {
        logWarning("CLightList::evalPdfImportant", "Could not find light");
        return 0.0;
    }

    // Prob for choosing this light
    if ( totalImp < EPSILON ) {
        pdf = 0.0;
    } else {
        pdf = info->importance / totalImp;
    }

    return pdf;
}
