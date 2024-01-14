#include <ctime>
#include <cstring>

#include "common/error.h"
#include "material/statistics.h"
#include "skin/radianceinterfaces.h"
#include "scene/scene.h"
#include "shared/writevrml.h"
#include "shared/options.h"
#include "shared/canvas.h"
#include "shared/render.h"
#include "io/FileUncompressWrapper.h"
#include "raycasting/common/Raytracer.h"
#include "app/batch.h"

static int iterations = 1; // radiance method iterations
static int save_modulo = 10; // every 10th iteration will be saved
static int timings = false;

static const char *radiance_image_filename_format = "";
static const char *radiance_model_filename_format = "";
static const char *raytracing_image_filename = "";

static CMDLINEOPTDESC batchOptions[] = {
    {"-iterations", 3,  Tint, &iterations, DEFAULT_ACTION,
     "-iterations <integer>\t: world-space radiance iterations"},
    {"-radiance-image-savefile", 12, Tstring,
     &radiance_image_filename_format, DEFAULT_ACTION,
     "-radiance-image-savefile <filename>\t: radiance PPM/LOGLUV savefile name,"
     "\n\tfirst '%%d' will be substituted by iteration number"},
    {"-radiance-model-savefile", 12, Tstring,
     &radiance_model_filename_format, DEFAULT_ACTION,
     "-radiance-model-savefile <filename>\t: radiance VRML model savefile name,"
     "\n\tfirst '%%d' will be substituted by iteration number"},
    {"-save-modulo", 8, Tint, &save_modulo, DEFAULT_ACTION,
     "-save-modulo <integer>\t: save every n-th iteration"},
    {"-raytracing-image-savefile", 14, Tstring, &raytracing_image_filename, DEFAULT_ACTION,
     "-raytracing-image-savefile <filename>\t: raytracing PPM savefile name"},
    {"-timings", 3,  Tsettrue, &timings, DEFAULT_ACTION,
     "-timings\t: print timings for world-space radiance and raytracing methods"},
    {nullptr, 0,  TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
This routine was copied from uit.c, leaving out all interface related things
*/
static void
BatchProcessFile(const char *filename, const char *open_mode,
                 void (*process_file)(const char *filename, FILE *fp, int ispipe)) {
    int ispipe;
    FILE *fp = OpenFile(filename, open_mode, &ispipe);

    /* call the user supplied procedure to process the file */
    process_file(filename, fp, ispipe);

    CloseFile(fp, ispipe);
}

static void
BatchSaveRadianceImage(const char *fname, FILE *fp, int ispipe) {
    clock_t t;
    char *extension;

    if ( !fp ) {
        return;
    }

    CanvasPushMode();

    extension = ImageFileExtension((char *)fname);
    if ( IS_TIFF_LOGLUV_EXT(extension)) {
        fprintf(stdout, "Saving LOGLUV image to file '%s' ....... ", fname);
    } else {
        fprintf(stdout, "Saving RGB image to file '%s' .......... ", fname);
    }
    fflush(stdout);

    t = clock();

    saveScreen((char *) fname, fp, ispipe);

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    CanvasPullMode();
}

static void
BatchSaveRadianceModel(const char *fname, FILE *fp, int ispipe) {
    clock_t t;

    if ( !fp ) {
        return;
    }

    CanvasPushMode();
    fprintf(stdout, "Saving VRML model to file '%s' ... ", fname);
    fflush(stdout);
    t = clock();

    if ( GLOBAL_radiance_currentRadianceMethodHandle && GLOBAL_radiance_currentRadianceMethodHandle->WriteVRML ) {
        GLOBAL_radiance_currentRadianceMethodHandle->WriteVRML(fp);
    } else {
        WriteVRML(fp);
    }

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    CanvasPullMode();
}

static void
BatchSaveRaytracingImage(const char *fname, FILE *fp, int ispipe) {
    ImageOutputHandle *img = nullptr;
    clock_t t;

    if ( !fp ) {
        return;
    }

    t = clock();

    if ( fp ) {
        img = CreateRadianceImageOutputHandle(
	    (char *)fname,
	    fp,
	    ispipe,
            GLOBAL_camera_mainCamera.hres,
	    GLOBAL_camera_mainCamera.vres,
	    GLOBAL_statistics_referenceLuminance / 179.0);
        if ( !img ) {
            return;
        }
    }

    if ( !Global_Raytracer_activeRaytracer ) {
        logWarning(nullptr, "No ray tracing method active");
    } else if ( !Global_Raytracer_activeRaytracer->SaveImage || !Global_Raytracer_activeRaytracer->SaveImage(img)) {
        logWarning(nullptr, "No previous %s image available", Global_Raytracer_activeRaytracer->fullName);
    }

    if ( img ) {
        DeleteImageOutputHandle(img);
    }

    fprintf(stdout, "Raytrace save image: %g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);

    return;
}

static void
BatchRayTrace(char *filename, FILE *fp, int ispipe) {
    GLOBAL_render_renderOptions.render_raytraced_image = true;
    GLOBAL_camera_mainCamera.changed = false;

    CanvasPushMode();
    RayTrace(filename, fp, ispipe);
    CanvasPullMode();
}

void
parseBatchOptions(int *argc, char **argv) {
    parseOptions(batchOptions, argc, argv);
}

void
batch() {
    clock_t start_time, wasted_start;
    float wasted_secs;

    if ( !GLOBAL_scene_world ) {
        printf("Empty world??\n");
        return;
    }

    start_time = clock();
    wasted_secs = 0.0;

    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        /* GLOBAL_scene_world-space radiance computations */
        int it = 0, done = false;

        printf("Doing %s ...\n", GLOBAL_radiance_currentRadianceMethodHandle->fullName);

        fflush(stdout);
        fflush(stderr);

        while ( !done ) {
            printf("-----------------------------------\n"
                   "GLOBAL_scene_world-space radiance iteration %04d\n"
                   "-----------------------------------\n\n", it);

            CanvasPushMode();
            done = GLOBAL_radiance_currentRadianceMethodHandle->DoStep();
            CanvasPullMode();

            fflush(stdout);
            fflush(stderr);

            printf("%s", GLOBAL_radiance_currentRadianceMethodHandle->GetStats());

            fflush(stdout);
            fflush(stderr);

            renderScene();

            fflush(stdout);
            fflush(stderr);

            wasted_start = clock();

            if ( (!(it % save_modulo)) && *radiance_image_filename_format ) {
                char *fname = (char *)malloc(strlen(radiance_image_filename_format) + 1);
                sprintf(fname, radiance_image_filename_format, it);
                if ( GLOBAL_render_renderOptions.trace ) {
                    char *dot;
                    char *tmpName;
                    const char *tiffExt = "tif";

                    BatchProcessFile(fname, "w", BatchSaveRadianceImage);

                    /* NOw, change the extenstion to '.tiff' and save it as RGB. */

                    tmpName = (char *)malloc(strlen(fname) + strlen(tiffExt) + 1);
                    strcpy(tmpName, fname);
                    dot = ImageFileExtension(tmpName);
                    if ( dot ) {
                        *dot = '\0';
                    }
                    strcat(tmpName, tiffExt);
                    BatchProcessFile(tmpName, "w", BatchSaveRadianceImage);
                    free(tmpName);
                } else {
                    BatchProcessFile(fname, "w", BatchSaveRadianceImage);
                }
            }

            if ( *radiance_model_filename_format ) {
                char *fname = (char *)malloc(strlen(radiance_model_filename_format) + 1);
                sprintf(fname, radiance_model_filename_format, it);
                BatchProcessFile(fname, "w", BatchSaveRadianceModel);
                free(fname);
            }

            wasted_secs += (float) (wasted_start - clock()) / (float) CLOCKS_PER_SEC;

            fflush(stdout);
            fflush(stderr);

            it++;
            if ( iterations > 0 && it >= iterations ) {
                done = true;
            }
        }
    } else {
        printf("(No world-space radiance computations are being done)\n");
    }

    if ( timings ) {
        fprintf(stdout, "Radiance total time %g secs.\n",
                ((float) (clock() - start_time) / (float) CLOCKS_PER_SEC) - wasted_secs);
    }

    if ( Global_Raytracer_activeRaytracer ) {
        printf("Doing %s ...\n", Global_Raytracer_activeRaytracer->fullName);

        start_time = clock();
        BatchRayTrace(nullptr, nullptr, false);
        if ( timings ) {
            fprintf(stdout, "Raytracing total time %g secs.\n",
                    (float) (clock() - start_time) / (float) CLOCKS_PER_SEC);
        }

        BatchProcessFile(raytracing_image_filename, "w", BatchSaveRaytracingImage);
    } else {
        printf("(No pixel-based radiance computations are being done)\n");
    }

    printf("Computations finished.\n");
}
