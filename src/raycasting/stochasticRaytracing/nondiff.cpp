/* non diffuse first shot */

#include <cstdlib>

#include "material/statistics.h"
#include "material/edf.h"
#include "scene/scene.h"
#include "shared/render.h"
#include "raycasting/stochasticRaytracing/localline.h"
#include "raycasting/stochasticRaytracing/mcradP.h"

class LIGHTSOURCETABLE {
  public:
    PATCH *patch;
    double flux;
};

static LIGHTSOURCETABLE *lights;

static int nrlights, nrsamples;
static double totalflux;

static void InitLight(LIGHTSOURCETABLE *light, PATCH *l, double flux) {
    light->patch = l;
    light->flux = flux;
}

static void MakeLightSourceTable() {
    int i = 0;
    totalflux = 0.;
    nrlights = GLOBAL_statistics_numberOfLightSources;
    lights = (LIGHTSOURCETABLE *)malloc(nrlights * sizeof(LIGHTSOURCETABLE));
    ForAllPatches(l, GLOBAL_scene_lightSourcePatches)
                {
                    COLOR emitted_rad = PatchAverageEmittance(l, ALL_COMPONENTS);
                    double flux = M_PI * l->area * colorSumAbsComponents(emitted_rad);
                    totalflux += flux;
                    InitLight(&lights[i], l, flux);
                    i++;
                }
    EndForAll;
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    stochasticRadiosityClearCoefficients(getTopLevelPatchRad(P), getTopLevelPatchBasis(P));
                    stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));
                    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
                    colorClear(TOPLEVEL_ELEMENT(P)->source_rad);
                }
    EndForAll;
}

static void NextLightSample(PATCH *patch, double *zeta) {
    double *xi = Sample4D(TOPLEVEL_ELEMENT(patch)->ray_index++);
    if ( patch->nrvertices == 3 ) {
        double u = xi[0], v = xi[1];
        FoldSampleF(&u, &v);
        zeta[0] = u;
        zeta[1] = v;
    } else {
        zeta[0] = xi[0];
        zeta[1] = xi[1];
    }
    zeta[2] = xi[2];
    zeta[3] = xi[3];
}

static Ray SampleLightRay(PATCH *patch, COLOR *emitted_rad, double *point_selection_pdf, double *dir_selection_pdf) {
    Ray ray;
    do {
        double zeta[4];
        HITREC hit;
        NextLightSample(patch, zeta);

        PatchUniformPoint(patch, zeta[0], zeta[1], &ray.pos);

        InitHit(&hit, patch, (Geometry *) 0, &ray.pos, &patch->normal, patch->surface->material, 0.);
        ray.dir = EdfSample(patch->surface->material->edf, &hit, ALL_COMPONENTS, zeta[2], zeta[3], emitted_rad,
                            dir_selection_pdf);
    } while ( *dir_selection_pdf == 0. );

    /* TODO: the following is only correct if no rejections would result in the */
    /* loop above, i.o.w. the surface is not textured, or it is textured, but there */
    /* are no areas that are non-selfemitting. */
    *point_selection_pdf = 1. / patch->area;  /* uniform area sampling */
    return ray;
}

static void SampleLight(LIGHTSOURCETABLE *light, double light_selection_pdf) {
    COLOR rad;
    double point_selection_pdf, dir_selection_pdf;
    Ray ray = SampleLightRay(light->patch, &rad, &point_selection_pdf, &dir_selection_pdf);
    HITREC hitstore, *hit;

    mcr.traced_rays++;
    hit = McrShootRay(light->patch, &ray, &hitstore);
    if ( hit ) {
        double pdf = light_selection_pdf * point_selection_pdf * dir_selection_pdf;
        double outcos = VECTORDOTPRODUCT(ray.dir, light->patch->normal);
        COLOR rcvrad, Rd = TOPLEVEL_ELEMENT(hit->patch)->Rd;
        colorScale((outcos / (M_PI * hit->patch->area * pdf * nrsamples)), rad, rcvrad);
        colorProduct(Rd, rcvrad, rcvrad);
        colorAdd(getTopLevelPatchRad(hit->patch)[0], rcvrad, getTopLevelPatchRad(hit->patch)[0]);
        colorAdd(getTopLevelPatchUnShotRad(hit->patch)[0], rcvrad, getTopLevelPatchUnShotRad(hit->patch)[0]);
        colorAdd(TOPLEVEL_ELEMENT(hit->patch)->source_rad, rcvrad, TOPLEVEL_ELEMENT(hit->patch)->source_rad);
    }
}

static void SampleLightSources(int nr_samples) {
    double rnd = drand48();
    int count = 0, i;
    double p_cumul = 0.;
    nrsamples = nr_samples;
    fprintf(stderr, "Shooting %d light rays ", nrsamples);
    fflush(stderr);
    for ( i = 0; i < nrlights; i++ ) {
        int j;
        double p = lights[i].flux / totalflux;
        int samples_this_light =
                (int) floor((p_cumul + p) * (double) nrsamples + rnd) - count;

        for ( j = 0; j < samples_this_light; j++ ) {
            SampleLight(&lights[i], p);
        }

        p_cumul += p;
        count += samples_this_light;
    }

    fputc('\n', stderr);
}

static void Summarize() {
    colorClear(mcr.unshot_flux);
    mcr.unshot_ymp = 0.;
    colorClear(mcr.total_flux);
    mcr.total_ymp = 0.;
    colorClear(mcr.imp_unshot_flux);
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    colorAddScaled(mcr.unshot_flux, M_PI * P->area, getTopLevelPatchUnShotRad(P)[0], mcr.unshot_flux);
                    colorAddScaled(mcr.total_flux, M_PI * P->area, getTopLevelPatchRad(P)[0], mcr.total_flux);
                    colorAddScaled(mcr.imp_unshot_flux, M_PI * P->area * (TOPLEVEL_ELEMENT(P)->imp - TOPLEVEL_ELEMENT(P)->source_imp), getTopLevelPatchUnShotRad(P)[0],
                                   mcr.imp_unshot_flux);
                    mcr.unshot_ymp += P->area * fabs(TOPLEVEL_ELEMENT(P)->unshot_imp);
                    mcr.total_ymp += P->area * TOPLEVEL_ELEMENT(P)->imp;
                    mcr.source_ymp += P->area * TOPLEVEL_ELEMENT(P)->source_imp;
                    monteCarloRadiosityPatchComputeNewColor(P);
                }
    EndForAll;
}

/**
Initial shooting pass handling non-diffuse light sources
*/
void
doNonDiffuseFirstShot() {
    MakeLightSourceTable();
    SampleLightSources(mcr.initial_ls_samples * nrlights);
    Summarize();
    RenderScene();
}
