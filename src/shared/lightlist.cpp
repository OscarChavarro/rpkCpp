#include <cmath>

#include "shared/lightlist.h"
#include "skin/Patch.h"
#include "material/color.h"
#include "common/error.h"

/* *** GLOBALS *** */

CLightList *gLightList = nullptr;

/* *** C METHODS *** */

double LightEvalPDFImportant(Patch *light, Vector3D *lightPoint,
                             Vector3D *point, Vector3D *normal) {
    return gLightList->EvalPDFImportant(light, lightPoint,
                                        point, normal);
}


/* *** METHODS *** */

void CLightList::IncludeVirtualPatches(bool newValue) {
    includeVirtual = newValue;
}

CLightList::CLightList(java::ArrayList<Patch *> *list, bool includeVirtualPatches) {
    CLightInfo info;
    COLOR lightColor;
    {
        static int wgiv = 0;
        if ( !wgiv ) {
            logWarning("CLightList::CLightList", "not yet ready for texturing");
            wgiv = 1;
        }
    }

    totalFlux = 0.0;
    lightCount = 0;
    includeVirtual = includeVirtualPatches;

    for ( int i = 0; list != nullptr && i < list->size(); i++ ) {
        Patch *light = list->get(i);
        if ( !light->isVirtual() || includeVirtual ) {
            if ( light->surface->material->edf != nullptr ) {
                info.light = light;

                // Calculate emittedFlux
                if ( light->isVirtual()) {
                    COLOR e = EdfEmittance(light->surface->material->edf, nullptr, DIFFUSE_COMPONENT);
                    info.emittedFlux = colorAverage(e);
                } else {
                    lightColor = patchAverageEmittance(light, DIFFUSE_COMPONENT);
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


// Returns sampled patch, scales x_1 back to a random in 0..1

Patch *CLightList::Sample(double *x_1, double *pdf) {
    CLightInfo *info, *lastInfo;
    CTSList_Iter<CLightInfo> iterator(*this);

    double rnd = *x_1 * totalFlux;
    double currentSum;

    info = iterator.Next();
    while ((info != nullptr) && (info->light->isVirtual()) && (!includeVirtual) ) {
        info = iterator.Next();
    }

    if ( info == nullptr ) {
        logWarning("CLightList::Sample", "No lights available");
        return nullptr;
    }

    currentSum = info->emittedFlux;

    while ((rnd > currentSum) && (info != nullptr)) {
        lastInfo = info;
        info = iterator.Next();
        while ((info != nullptr) && (info->light->isVirtual()) && (!includeVirtual)) {
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
        *x_1 = ((*x_1 - ((currentSum - info->emittedFlux) / totalFlux)) /
                (info->emittedFlux / totalFlux));
        *pdf = info->emittedFlux / totalFlux;
        return (info->light);
    }

    return nullptr;
}

double CLightList::EvalPDF_virtual(Patch *light, Vector3D */*point*/) {
    // EvalPDF for virtual patches (see EvalPDF)
    double pdf;

    // Prob for choosing this light
    XXDFFLAGS all = DIFFUSE_COMPONENT || GLOSSY_COMPONENT || SPECULAR_COMPONENT;

    COLOR e = EdfEmittance(light->surface->material->edf, (RayHit *) nullptr, all);
    pdf = colorAverage(e) / totalFlux;

    return pdf;
}

double CLightList::EvalPDF_real(Patch *light, Vector3D */*point*/) {
    // Eval PDF for normal patches (see EvalPDF)
    COLOR col;
    double pdf;

    col = patchAverageEmittance(light, DIFFUSE_COMPONENT);

    // Prob for choosing this light
    pdf = colorAverage(col) * light->area / totalFlux;

    return pdf;
}

double CLightList::EvalPDF(Patch *light, Vector3D *point) {
    // TODO!!!  1) patch should become class
    //          2) virtual patch should become child-class
    //          3) this method should be handled by specialisation
    if ( totalFlux < EPSILON ) {
        return 0.0;
    }
    if ( light->isVirtual() ) {
        return EvalPDF_virtual(light, point);
    } else {
        return EvalPDF_real(light, point);
    }
}



/*************************************************************************/
/* Important light sampling */

double CLightList::ComputeOneLightImportance_virtual(Patch *light,
                                                     const Vector3D *,
                                                     const Vector3D *,
                                                     float) {
    // ComputeOneLightImportance for virtual patches
    XXDFFLAGS all = DIFFUSE_COMPONENT || GLOSSY_COMPONENT || SPECULAR_COMPONENT;

    COLOR e = EdfEmittance(light->surface->material->edf, (RayHit *) nullptr, all);
    return colorAverage(e);
}

double CLightList::ComputeOneLightImportance_real(Patch *light,
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

        // ray direction (but no ray is shot of course)
        Vector3D copy(point->x, point->y, point->z);
	
        VECTORSUBTRACT(lightPoint, copy, dir);
        dist2 = VECTORNORM2(dir);
        // VECTORSCALE(1/dist, dir, dir);

        /* Check normals */

        // Cosines have an addition dist length in them

        cosRayLight = -VECTORDOTPRODUCT(dir, light_normal);
        cosRayPatch = VECTORDOTPRODUCT(dir, *normal);

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

double CLightList::ComputeOneLightImportance(Patch *light,
                                             const Vector3D *point,
                                             const Vector3D *normal,
                                             float emittedFlux) {
    // TODO!!!  1) patch should become class
    //          2) virtual patch should become child-class
    //          3) this method should be handled by specialisation
    if ( light->isVirtual()) {
        return
                ComputeOneLightImportance_virtual(light, point, normal, emittedFlux);
    } else {
        return
                ComputeOneLightImportance_real(light, point, normal, emittedFlux);
    }

}

void CLightList::ComputeLightImportances(Vector3D *point, Vector3D *normal) {
    if ((VECTOREQUAL(*point, lastPoint, EPSILON)) &&
        (VECTOREQUAL(*normal, lastNormal, EPSILON))) {
        return; // Still ok !!
    }


    CLightInfo *info;
    CTSList_Iter<CLightInfo> iterator(*this);
    double imp;

    totalImp = 0.0;

    // next
    info = iterator.Next();
    while ((info != nullptr) && (info->light->isVirtual()) && (!includeVirtual)) {
        info = iterator.Next();
    }

    while ( info ) {
        imp = ComputeOneLightImportance(info->light, point, normal,
                                        info->emittedFlux);
        totalImp += imp;
        info->importance = imp;

        // next
        info = iterator.Next();
        while ((info != nullptr) && (info->light->isVirtual()) && (!includeVirtual)) {
            info = iterator.Next();
        }
    }
}

Patch *CLightList::SampleImportant(Vector3D *point, Vector3D *normal,
                                   double *x_1, double *pdf) {
    CLightInfo *info, *lastInfo;
    CTSList_Iter<CLightInfo> iterator(*this);
    double rnd, currentSum;

    ComputeLightImportances(point, normal);

    if ( totalImp == 0 ) {
        // No light is important, but we must return one (->optimize ?)
        return (Sample(x_1, pdf));
    }

    rnd = *x_1 * totalImp;

    // next
    info = iterator.Next();
    while ((info != nullptr) && (info->light->isVirtual()) && (!includeVirtual)) {
        info = iterator.Next();
    }

    if ( info == nullptr ) {
        logWarning("CLightList::Sample", "No lights available");
        return nullptr;
    }

    currentSum = info->importance;

    while ((rnd > currentSum) && (info != nullptr)) {
        lastInfo = info;

        // next
        info = iterator.Next();
        while ( info != nullptr && info->light->isVirtual() && !includeVirtual ) {
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
        *x_1 = ((*x_1 - ((currentSum - info->importance) / totalImp)) /
                (info->importance / totalImp));
        *pdf = info->importance / totalImp;
        return (info->light);
    }

    return nullptr;
}

double CLightList::EvalPDFImportant(Patch *light, Vector3D */*lightPoint*/,
                                    Vector3D *litPoint, Vector3D *normal) {
    double pdf;
    CLightInfo *info;
    CTSList_Iter<CLightInfo> iterator(*this);

    ComputeLightImportances(litPoint, normal);

    // Search the light in the list :-(

    while ((info = iterator.Next()) && info->light != light ) {
    }

    if ( info == nullptr ) {
        logWarning("CLightList::EvalPDFImportant", "Could not find light");
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

