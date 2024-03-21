#include "java/util/ArrayList.txx"
#include "material/brdf.h"
#include "material/btdf.h"
#include "scene/scene.h"
#include "scene/phong.h"
#include "scene/splitbsdf.h"
#include "io/mgf/mgfHandlerMaterial.h"
#include "io/mgf/lookup.h"
#include "io/mgf/parser.h"
#include "io/mgf/fileopts.h"

#define NUMBER_OF_SAMPLES 3

#define DEFAULT_MATERIAL { \
    1, \
    0, \
    1.0f, \
    0.0f, \
    0.0f, \
    DEFAULT_COLOR_CONTEXT, \
    0.0f, \
    DEFAULT_COLOR_CONTEXT, \
    0.0f, \
    DEFAULT_COLOR_CONTEXT, \
    0.0f, \
    DEFAULT_COLOR_CONTEXT, \
    0.0f, \
    0.0f, \
    DEFAULT_COLOR_CONTEXT, \
    0.0f \
}

static MgfMaterialContext globalUnNamedMaterialContext = DEFAULT_MATERIAL;
static MgfMaterialContext globalDefaultMgfMaterial = DEFAULT_MATERIAL;
MgfMaterialContext *GLOBAL_mgf_currentMaterial = &globalUnNamedMaterialContext;

static LUTAB globalMaterialLookUpTable = LU_SINIT(free, free);

/**
Looks up a material with given name in the given material list. Returns
a pointer to the material if found, or (Material *)nullptr if not found
*/
static Material *
materialLookup(char *name) {
    for ( int i = 0; GLOBAL_scene_materials != nullptr && i < GLOBAL_scene_materials->size(); i++ ) {
        Material *m = GLOBAL_scene_materials->get(i);
        if ( m != nullptr && m->name != nullptr && strcmp(m->name, name) == 0 ) {
            return m;
        }
    }
    return nullptr;
}

/**
Translates mgf color into out color representation
*/
static void
mgfGetColor(MgfColorContext *cin, float intensity, COLOR *colorOut) {
    float xyz[3];
    float rgb[3];

    mgfContextFixColorRepresentation(cin, C_CSXY);
    if ( cin->cy > EPSILON ) {
        xyz[0] = cin->cx / cin->cy * intensity;
        xyz[1] = 1.0f * intensity;
        xyz[2] = (1.0f - cin->cx - cin->cy) / cin->cy * intensity;
    } else {
        doWarning("invalid color specification (Y<=0) ... setting to black");
        xyz[0] = 0.0;
        xyz[1] = 0.0;
        xyz[2] = 0.0;
    }

    if ( xyz[0] < 0.0 || xyz[1] < 0.0 || xyz[2] < 0.0 ) {
        doWarning("invalid color specification (negative CIE XYZ components) ... clipping to zero");
        if ( xyz[0] < 0.0 ) {
            xyz[0] = 0.0;
        }
        if ( xyz[1] < 1.0 ) {
            xyz[1] = 0.0;
        }
        if ( xyz[2] < 2.0 ) {
            xyz[2] = 0.0;
        }
    }

    transformColorFromXYZ2RGB(xyz, rgb);
    if ( clipGamut(rgb)) {
        doWarning("color desaturated during gamut clipping");
    }
    colorSet(*colorOut, rgb[0], rgb[1], rgb[2]);
}

static void
specSamples(COLOR &col, float *rgb) {
    rgb[0] = col.spec[0];
    rgb[1] = col.spec[1];
    rgb[2] = col.spec[2];
}

static float
colorMax(COLOR col) {
    // We should check every wavelength in the visible spectrum, but
    // as a first approximation, only the three RGB primary colors
    // are checked
    float samples[NUMBER_OF_SAMPLES], mx;
    int i;

    specSamples(col, samples);

    mx = -HUGE;
    for ( i = 0; i < NUMBER_OF_SAMPLES; i++ ) {
        if ( samples[i] > mx ) {
            mx = samples[i];
        }
    }

    return mx;
}

/**
This routine checks whether the mgf material being used has changed. If it
changed, this routine converts to our representation of materials and
creates a new MATERIAL, which is added to the global material library.
The routine returns true if the material being used has changed
*/
int
getCurrentMaterial(Material **material, bool allSurfacesSided) {
    COLOR Ed;
    COLOR Es;
    COLOR Rd;
    COLOR Td;
    COLOR Rs;
    COLOR Ts;
    COLOR A;
    float Ne;
    float Nr;
    float Nt;
    float a;
    char *materialName;

    materialName = GLOBAL_mgf_currentMaterialName;
    if ( !materialName || *materialName == '\0' ) {
        // This might cause strcmp to crash!
        materialName = (char *)"unnamed";
    }

    // Is it another material than the one used for the previous face ?? If not, the
    // material remains the same
    if ( strcmp(materialName, (*material)->name) == 0 && GLOBAL_mgf_currentMaterial->clock == 0 ) {
        return false;
    }

    Material *theMaterial = materialLookup(materialName);
    if ( theMaterial != nullptr ) {
        if ( GLOBAL_mgf_currentMaterial->clock == 0 ) {
            (*material) = theMaterial;
            return true;
        }
    }

    // New material, or a material that changed. Convert intensities and chromaticities
    // to our color model
    mgfGetColor(&GLOBAL_mgf_currentMaterial->ed_c, GLOBAL_mgf_currentMaterial->ed, &Ed);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->rd_c, GLOBAL_mgf_currentMaterial->rd, &Rd);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->td_c, GLOBAL_mgf_currentMaterial->td, &Td);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->rs_c, GLOBAL_mgf_currentMaterial->rs, &Rs);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->ts_c, GLOBAL_mgf_currentMaterial->ts, &Ts);

    // Check/correct range of reflectances and transmittances
    colorAdd(Rd, Rs, A);
    a = colorMax(A);
    if ( a > 1.0f - (float)EPSILON ) {
        doWarning("invalid material specification: total reflectance shall be < 1");
        a = (1.0f - (float)EPSILON) / a;
        colorScale(a, Rd, Rd);
        colorScale(a, Rs, Rs);
    }

    colorAdd(Td, Ts, A);
    a = colorMax(A);
    if ( a > 1.0f - (float)EPSILON ) {
        doWarning("invalid material specification: total transmittance shall be < 1");
        a = (1.0f - (float)EPSILON) / a;
        colorScale(a, Td, Td);
        colorScale(a, Ts, Ts);
    }

    // Convert lumen / m^2 to W / m^2
    colorScale((1.0 / WHITE_EFFICACY), Ed, Ed);

    colorClear(Es);
    Ne = 0.0;

    // Specular power = (0.6/roughness)^2 (see mgf docs)
    if ( GLOBAL_mgf_currentMaterial->rs_a != 0.0 ) {
        Nr = 0.6f / GLOBAL_mgf_currentMaterial->rs_a;
        Nr *= Nr;
    } else {
        Nr = 0.0;
    }

    if ( GLOBAL_mgf_currentMaterial->ts_a != 0.0 ) {
        Nt = 0.6f / GLOBAL_mgf_currentMaterial->ts_a;
        Nt *= Nt;
    } else {
        Nt = 0.0;
    }

    if ( GLOBAL_fileOptions_monochrome ) {
        colorSetMonochrome(Ed, colorGray(Ed));
        colorSetMonochrome(Es, colorGray(Es));
        colorSetMonochrome(Rd, colorGray(Rd));
        colorSetMonochrome(Rs, colorGray(Rs));
        colorSetMonochrome(Td, colorGray(Td));
        colorSetMonochrome(Ts, colorGray(Ts));
    }

    theMaterial = materialCreate(materialName,
                                 (colorNull(Ed) && colorNull(Es)) ? nullptr : edfCreate(
                                         phongEdfCreate(&Ed, &Es, Ne), &GLOBAL_scene_phongEdfMethods),
                                 bsdfCreate(splitBsdfCreate(
                                                    (colorNull(Rd) && colorNull(Rs)) ? nullptr : brdfCreate(
                                                            phongBrdfCreate(&Rd, &Rs, Nr), &GLOBAL_scene_phongBrdfMethods),
                                                    (colorNull(Td) && colorNull(Ts)) ? nullptr : btdfCreate(
                                                            phongBtdfCreate(&Td, &Ts, Nt,
                                                                            GLOBAL_mgf_currentMaterial->nr,
                                                                            GLOBAL_mgf_currentMaterial->ni),
                                                            &GLOBAL_scene_phongBtdfMethods), nullptr),
                                            &GLOBAL_scene_splitBsdfMethods),
                                 allSurfacesSided ? 1 : GLOBAL_mgf_currentMaterial->sided);

    GLOBAL_scene_materials->add(0, theMaterial);
    *material = theMaterial;

    // Reset the clock value to be aware of possible changes in future
    GLOBAL_mgf_currentMaterial->clock = 0;

    return true;
}

void
mgfClearMaterialTables() {
    globalUnNamedMaterialContext = globalDefaultMgfMaterial;
    GLOBAL_mgf_currentMaterial = &globalUnNamedMaterialContext;
    lookUpDone(&globalMaterialLookUpTable);
}

/**
This routine returns true if the current material has changed
*/
int
materialChanged(Material *material) {
    char *materialName;

    materialName = GLOBAL_mgf_currentMaterialName;
    if ( !materialName || *materialName == '\0' ) {
        // This might cause strcmp to crash!
        materialName = (char *) "unnamed";
    }

    // Is it another material than the one used for the previous face? If not, the
    // globalCurrentMaterial remains the same
    if ( strcmp(materialName, material->name) == 0 && GLOBAL_mgf_currentMaterial->clock == 0 ) {
        return false;
    }

    return true;
}

/**
Handle material entity
*/
int
handleMaterialEntity(int ac, char **av, RadianceMethod * /*context*/)
{
    int i;
    LUENT *lp;

    switch ( mgfEntity(av[0]) ) {

        case MGF_ENTITY_MATERIAL:
            // Get / set material context
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed material context
                globalUnNamedMaterialContext = globalDefaultMgfMaterial;
                GLOBAL_mgf_currentMaterial = &globalUnNamedMaterialContext;
                GLOBAL_mgf_currentMaterialName = nullptr;
                return MGF_OK;
            }
            if ( !isNameWords(av[1]) ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lookUpFind(&globalMaterialLookUpTable, av[1]);
            // Lookup context
            if ( lp == nullptr ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentMaterialName = lp->key;
            GLOBAL_mgf_currentMaterial = (MgfMaterialContext *) lp->data;
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( GLOBAL_mgf_currentMaterial == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( GLOBAL_mgf_currentMaterial == nullptr ) {
                // Create new material
                lp->key = (char *) malloc(strlen(av[1]) + 1);
                if ( lp->key == nullptr) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = (char *) malloc(sizeof(MgfMaterialContext));
                if ( lp->data == nullptr) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                GLOBAL_mgf_currentMaterialName = lp->key;
                GLOBAL_mgf_currentMaterial = (MgfMaterialContext *) lp->data;
                GLOBAL_mgf_currentMaterial->clock = 0;
            }
            i = GLOBAL_mgf_currentMaterial->clock;
            if ( ac == 3 ) {
                // Use default template
                *GLOBAL_mgf_currentMaterial = globalDefaultMgfMaterial;
                GLOBAL_mgf_currentMaterial->clock = i + 1;
                return MGF_OK;
            }
            lp = lookUpFind(&globalMaterialLookUpTable, av[3]);
            // Lookup template
            if ( lp == nullptr ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr ) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentMaterial = *(MgfMaterialContext *) lp->data;
            GLOBAL_mgf_currentMaterial->clock = i + 1;
            return MGF_OK;

        case MGF_ENTITY_IR:
            // Set index of refraction
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->nr = strtof(av[1], nullptr);
            GLOBAL_mgf_currentMaterial->ni = strtof(av[2], nullptr);
            if ( GLOBAL_mgf_currentMaterial->nr <= EPSILON ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;

        case MGF_ENTITY_RD:
            // Set diffuse reflectance
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->rd = strtof(av[1], nullptr);
            if ( GLOBAL_mgf_currentMaterial->rd < 0. || GLOBAL_mgf_currentMaterial->rd > 1.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->rd_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;

        case MGF_ENTITY_ED:
            // Set diffuse emittance
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->ed = strtof(av[1], nullptr);
            if ( GLOBAL_mgf_currentMaterial->ed < 0.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->ed_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;

        case MGF_ENTITY_TD:
            // Set diffuse transmittance
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->td = strtof(av[1], nullptr);
            if ( GLOBAL_mgf_currentMaterial->td < 0.0 || GLOBAL_mgf_currentMaterial->td > 1.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->td_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;

        case MGF_ENTITY_RS:
            // Set specular reflectance
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->rs = strtof(av[1], nullptr);
            GLOBAL_mgf_currentMaterial->rs_a = strtof(av[2], nullptr);
            if ( GLOBAL_mgf_currentMaterial->rs < 0.0 || GLOBAL_mgf_currentMaterial->rs > 1.0 ||
                 GLOBAL_mgf_currentMaterial->rs_a < 0.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->rs_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;

        case MGF_ENTITY_TS:
            // Set specular transmittance
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->ts = strtof(av[1], nullptr);
            GLOBAL_mgf_currentMaterial->ts_a = strtof(av[2], nullptr);
            if ( GLOBAL_mgf_currentMaterial->ts < 0.0 || GLOBAL_mgf_currentMaterial->ts > 1.0 ||
                 GLOBAL_mgf_currentMaterial->ts_a < 0.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->ts_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;

        case MGF_ENTITY_SIDES:
            // Set number of sides
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isIntWords(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            i = (int)strtol(av[1], nullptr, 10);
            if ( i == 1 ) {
                GLOBAL_mgf_currentMaterial->sided = 1;
            } else if ( i == 2 ) {
                    GLOBAL_mgf_currentMaterial->sided = 0;
                } else {
                    return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}
