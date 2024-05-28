#include <cstring>

#include "java/util/ArrayList.txx"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "io/mgf/mgfDefinitions.h"
#include "io/mgf/lookup.h"
#include "io/mgf/words.h"
#include "io/mgf/MgfMaterialContext.h"
#include "io/mgf/mgfHandlerMaterial.h"

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
static MgfMaterialContext *globalMgfCurrentMaterial = &globalUnNamedMaterialContext;
static LookUpTable globalMaterialLookUpTable = LOOK_UP_INIT(lookUpRemove, lookUpRemove);

/**
Looks up a material with given name in the given material list. Returns
a pointer to the material if found, or nullptr if not found
*/
static Material *
materialLookup(const char *name, const MgfContext *context) {
    for ( int i = 0; context->materials != nullptr && i < context->materials->size(); i++ ) {
        Material *m = context->materials->get(i);
        if ( m != nullptr && m->getName() != nullptr && strcmp(m->getName(), name) == 0 ) {
            return m;
        }
    }
    return nullptr;
}

/**
Translates mgf color into out color representation
*/
static void
mgfGetColor(MgfColorContext *cin, float intensity, ColorRgb *colorOut, MgfContext *context) {
    float xyz[3];
    float rgb[3];

    cin->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( cin->cy > Numeric::EPSILON ) {
        xyz[0] = cin->cx / cin->cy * intensity;
        xyz[1] = 1.0f * intensity;
        xyz[2] = (1.0f - cin->cx - cin->cy) / cin->cy * intensity;
    } else {
        doWarning("invalid color specification (Y<=0) ... setting to black", context);
        xyz[0] = 0.0;
        xyz[1] = 0.0;
        xyz[2] = 0.0;
    }

    if ( xyz[0] < 0.0 || xyz[1] < 0.0 || xyz[2] < 0.0 ) {
        doWarning("invalid color specification (negative CIE XYZ components) ... clipping to zero", context);
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
    if ( clipGamut(rgb) ) {
        doWarning("color desaturated during gamut clipping", context);
    }
    colorOut->set(rgb[0], rgb[1], rgb[2]);
}

static void
specSamples(const ColorRgb &col, float *rgb) {
    rgb[0] = col.r;
    rgb[1] = col.g;
    rgb[2] = col.b;
}

static float
colorMax(ColorRgb col) {
    // We should check every wavelength in the visible spectrum, but
    // as a first approximation, only the three RGB primary colors
    // are checked
    float samples[NUMBER_OF_SAMPLES];
    float mx;

    specSamples(col, samples);

    mx = -Numeric::HUGE_FLOAT_VALUE;
    for ( int i = 0; i < NUMBER_OF_SAMPLES; i++ ) {
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
mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, MgfContext *context) {
    ColorRgb Ed;
    ColorRgb Es;
    ColorRgb Rd;
    ColorRgb Td;
    ColorRgb Rs;
    ColorRgb Ts;
    ColorRgb A;
    const char *materialName = context->currentMaterialName;
    if ( !materialName || *materialName == '\0' ) {
        // This might cause strcmp to crash!
        materialName = "unnamed";
    }

    // Is it another material than the one used for the previous face ?? If not, the
    // material remains the same
    if ( strcmp(materialName, (*material)->getName()) == 0 && globalMgfCurrentMaterial->clock == 0 ) {
        return false;
    }

    Material *storedMaterial = materialLookup(materialName, context);
    if ( storedMaterial != nullptr && globalMgfCurrentMaterial->clock == 0 ) {
        *material = storedMaterial;
        return true;
    }

    // New material, or a material that changed. Convert intensities and chromaticities
    // to our color model
    mgfGetColor(&globalMgfCurrentMaterial->ed_c, globalMgfCurrentMaterial->ed, &Ed, context);
    mgfGetColor(&globalMgfCurrentMaterial->rd_c, globalMgfCurrentMaterial->rd, &Rd, context);
    mgfGetColor(&globalMgfCurrentMaterial->td_c, globalMgfCurrentMaterial->td, &Td, context);
    mgfGetColor(&globalMgfCurrentMaterial->rs_c, globalMgfCurrentMaterial->rs, &Rs, context);
    mgfGetColor(&globalMgfCurrentMaterial->ts_c, globalMgfCurrentMaterial->ts, &Ts, context);

    // Check/correct range of reflectances and transmittances
    A.add(Rd, Rs);
    float a = colorMax(A);
    if ( a > 1.0f - Numeric::EPSILON_FLOAT ) {
        doWarning("invalid material specification: total reflectance shall be < 1", context);
        a = (1.0f - Numeric::EPSILON_FLOAT) / a;
        Rd.scale(a);
        Rs.scale(a);
    }

    A.add(Td, Ts);
    a = colorMax(A);
    if ( a > 1.0f - Numeric::EPSILON_FLOAT ) {
        doWarning("invalid material specification: total transmittance shall be < 1", context);
        a = (1.0f - Numeric::EPSILON_FLOAT) / a;
        Td.scale(a);
        Ts.scale(a);
    }

    // Convert lumen / m^2 to W / m^2
    Ed.scale(1.0f / WHITE_EFFICACY);

    Es.clear();

    float Ne = 0.0;
    float Nr;
    float Nt;

    // Specular power = (0.6/roughness)^2 (see mgf docs)
    if ( globalMgfCurrentMaterial->rs_a != 0.0 ) {
        Nr = 0.6f / globalMgfCurrentMaterial->rs_a;
        Nr *= Nr;
    } else {
        Nr = 0.0;
    }

    if ( globalMgfCurrentMaterial->ts_a != 0.0 ) {
        Nt = 0.6f / globalMgfCurrentMaterial->ts_a;
        Nt *= Nt;
    } else {
        Nt = 0.0;
    }

    if ( context->monochrome ) {
        Ed.setMonochrome(Ed.gray());
        Es.setMonochrome(Es.gray());
        Rd.setMonochrome(Rd.gray());
        Rs.setMonochrome(Rs.gray());
        Td.setMonochrome(Td.gray());
        Ts.setMonochrome(Ts.gray());
    }

    PhongEmittanceDistributionFunction* edf = nullptr;
    if ( !Ed.isBlack() || !Es.isBlack() ) {
        edf = new PhongEmittanceDistributionFunction(&Ed, &Es, Ne);
    }

    PhongBidirectionalReflectanceDistributionFunction *brdf = nullptr;
    if ( !Rd.isBlack() || !Rs.isBlack() ) {
        brdf = new PhongBidirectionalReflectanceDistributionFunction(&Rd, &Rs, Nr);
    }

    PhongBidirectionalTransmittanceDistributionFunction *btdf = nullptr;
    if ( !Td.isBlack() || !Ts.isBlack() ) {
        btdf = new PhongBidirectionalTransmittanceDistributionFunction(
            &Td,
            &Ts,
            Nt,
            globalMgfCurrentMaterial->nr,
            globalMgfCurrentMaterial->ni);
    }

    PhongBidirectionalScatteringDistributionFunction *bsdf = new PhongBidirectionalScatteringDistributionFunction(brdf, btdf, nullptr);

    (*material) = new Material(
        materialName,
         edf,
         bsdf,
        allSurfacesSided || globalMgfCurrentMaterial->sided);

    context->materials->add((*material));

    // Reset the clock value to be aware of possible changes in future
    globalMgfCurrentMaterial->clock = 0;

    return true;
}

void
initMaterialContextTables(MgfContext *context) {
    globalUnNamedMaterialContext = globalDefaultMgfMaterial;
    globalMgfCurrentMaterial = &globalUnNamedMaterialContext;
    lookUpDone(&globalMaterialLookUpTable);
    context->currentMaterialName = nullptr;
}

/**
This routine returns true if the current material has changed
*/
int
mgfMaterialChanged(const Material *material, const MgfContext *context) {
    const char *materialName = context->currentMaterialName;
    if ( materialName == nullptr || materialName[0] == '\0' ) {
        materialName = "unnamed";
    }

    // Is it another material than the one used for the previous face? If not, the
    // globalCurrentMaterial remains the same
    if ( strcmp(materialName, material->getName()) == 0 && globalMgfCurrentMaterial->clock == 0 ) {
        return false;
    }

    return true;
}

/**
Handle material entity
*/
int
handleMaterialEntity(int ac, const char **av, MgfContext *context) {
    int i;
    LookUpEntity *lp;

    switch ( mgfEntity(av[0], context) ) {

        case MgfEntity::MGF_MATERIAL:
            // Get / set material context
            if ( ac > 4 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed material context
                globalUnNamedMaterialContext = globalDefaultMgfMaterial;
                globalMgfCurrentMaterial = &globalUnNamedMaterialContext;
                context->currentMaterialName = nullptr;
                return MgfErrorCode::MGF_OK;
            }
            if ( !isNameWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lookUpFind(&globalMaterialLookUpTable, av[1]);
            // Lookup context
            if ( lp == nullptr ) {
                return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentMaterialName = lp->key;
            globalMgfCurrentMaterial = (MgfMaterialContext *) lp->data;
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( globalMgfCurrentMaterial == nullptr) {
                    return MgfErrorCode::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MgfErrorCode::MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( globalMgfCurrentMaterial == nullptr ) {
                // Create new material
                lp->key = new char[strlen(av[1]) + 1];
                if ( lp->key == nullptr) {
                    return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = new char[sizeof(MgfMaterialContext)];
                if ( lp->data == nullptr) {
                    return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
                }
                context->currentMaterialName = lp->key;
                globalMgfCurrentMaterial = (MgfMaterialContext *)lp->data;
                globalMgfCurrentMaterial->clock = 0;
            }
            i = globalMgfCurrentMaterial->clock;
            if ( ac == 3 ) {
                // Use default template
                *globalMgfCurrentMaterial = globalDefaultMgfMaterial;
                globalMgfCurrentMaterial->clock = i + 1;
                return MgfErrorCode::MGF_OK;
            }
            lp = lookUpFind(&globalMaterialLookUpTable, av[3]);
            // Lookup template
            if ( lp == nullptr ) {
                return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr ) {
                return MgfErrorCode::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *globalMgfCurrentMaterial = *(MgfMaterialContext *) lp->data;
            globalMgfCurrentMaterial->clock = i + 1;
            return MgfErrorCode::MGF_OK;

        case MgfEntity::IR:
            // Set index of refraction
            if ( ac != 3 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            globalMgfCurrentMaterial->nr = strtof(av[1], nullptr);
            globalMgfCurrentMaterial->ni = strtof(av[2], nullptr);
            if ( globalMgfCurrentMaterial->nr <= Numeric::EPSILON ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            globalMgfCurrentMaterial->clock++;
            return MgfErrorCode::MGF_OK;

        case MgfEntity::RD:
            // Set diffuse reflectance
            if ( ac != 2 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            globalMgfCurrentMaterial->rd = strtof(av[1], nullptr);
            if ( globalMgfCurrentMaterial->rd < 0. || globalMgfCurrentMaterial->rd > 1.0 ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            globalMgfCurrentMaterial->rd_c = *(context->currentColor);
            globalMgfCurrentMaterial->clock++;
            return MgfErrorCode::MGF_OK;

        case MgfEntity::ED:
            // Set diffuse emittance
            if ( ac != 2 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            globalMgfCurrentMaterial->ed = strtof(av[1], nullptr);
            if ( globalMgfCurrentMaterial->ed < 0.0 ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            globalMgfCurrentMaterial->ed_c = *(context->currentColor);
            globalMgfCurrentMaterial->clock++;
            return MgfErrorCode::MGF_OK;

        case MgfEntity::TD:
            // Set diffuse transmittance
            if ( ac != 2 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            globalMgfCurrentMaterial->td = strtof(av[1], nullptr);
            if ( globalMgfCurrentMaterial->td < 0.0 || globalMgfCurrentMaterial->td > 1.0 ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            globalMgfCurrentMaterial->td_c = *(context->currentColor);
            globalMgfCurrentMaterial->clock++;
            return MgfErrorCode::MGF_OK;

        case MgfEntity::RS:
            // Set specular reflectance
            if ( ac != 3 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            globalMgfCurrentMaterial->rs = strtof(av[1], nullptr);
            globalMgfCurrentMaterial->rs_a = strtof(av[2], nullptr);
            if ( globalMgfCurrentMaterial->rs < 0.0 || globalMgfCurrentMaterial->rs > 1.0 ||
                 globalMgfCurrentMaterial->rs_a < 0.0 ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            globalMgfCurrentMaterial->rs_c = *(context->currentColor);
            globalMgfCurrentMaterial->clock++;
            return MgfErrorCode::MGF_OK;

        case MgfEntity::TS:
            // Set specular transmittance
            if ( ac != 3 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            globalMgfCurrentMaterial->ts = strtof(av[1], nullptr);
            globalMgfCurrentMaterial->ts_a = strtof(av[2], nullptr);
            if ( globalMgfCurrentMaterial->ts < 0.0 || globalMgfCurrentMaterial->ts > 1.0 ||
                 globalMgfCurrentMaterial->ts_a < 0.0 ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            globalMgfCurrentMaterial->ts_c = *(context->currentColor);
            globalMgfCurrentMaterial->clock++;
            return MgfErrorCode::MGF_OK;

        case MgfEntity::SIDES:
            // Set number of sides
            if ( ac != 2 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isIntWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            i = (int)strtol(av[1], nullptr, 10);
            if ( i == 1 ) {
                globalMgfCurrentMaterial->sided = true;
            } else if ( i == 2 ) {
                    globalMgfCurrentMaterial->sided = false;
                } else {
                    return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
            globalMgfCurrentMaterial->clock++;
            return MgfErrorCode::MGF_OK;

        default:
            break;
    }
    return MgfErrorCode::MGF_ERROR_UNKNOWN_ENTITY;
}
