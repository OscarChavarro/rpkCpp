#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#include <string.h>
#include <stdlib.h>

#include "java/util/ArrayList.txx"
#include "java/lang/Math.h"
#include "common/color/Cie.h"
#include "common/dataStructures/LookUpEntity.h"
#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfMaterialEntitySupport.h"
#include "io/context/MaterialContext.h"

static ColorRgb
addColor(const ColorRgb &a, const ColorRgb &b) {
    return ColorRgb(a.getR() + b.getR(), a.getG() + b.getG(), a.getB() + b.getB());
}

static ColorRgb
scaleColor(const ColorRgb &c, float scale) {
    return ColorRgb(c.getR() * scale, c.getG() * scale, c.getB() * scale);
}

static bool
isBlack(const ColorRgb &c) {
    return Math::abs(c.getR()) < Numeric::EPSILON
        && Math::abs(c.getG()) < Numeric::EPSILON
        && Math::abs(c.getB()) < Numeric::EPSILON;
}

/**
Looks up a material with given name in the given material list. Returns
a pointer to the material if found, or NULL if not found
*/
Material *
MgfMaterialEntitySupport::materialLookup(const char *name, const ParseRuntimeContext *context) {
    for ( int i = 0; context->materials != NULL && i < context->materials->size(); i++ ) {
        Material *m = context->materials->get(i);
        if ( m != NULL && m->getName() != NULL && strcmp(m->getName(), name) == 0 ) {
            return m;
        }
    }
    return NULL;
}

/**
Translates mgf color into out color representation
*/
void
MgfMaterialEntitySupport::mgfGetColor(ColorContext *cin, float intensity, ColorRgb *colorOut, ParseRuntimeContext *context) {
    float xyz[3];
    float rgb[3];

    cin->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( cin->cy > Numeric::EPSILON ) {
        xyz[0] = cin->cx / cin->cy * intensity;
        xyz[1] = 1.0f * intensity;
        xyz[2] = (1.0f - cin->cx - cin->cy) / cin->cy * intensity;
    } else {
        MgfEntityControl::doWarning("invalid color specification (Y<=0) ... setting to black", context);
        xyz[0] = 0.0;
        xyz[1] = 0.0;
        xyz[2] = 0.0;
    }

    if ( xyz[0] < 0.0 || xyz[1] < 0.0 || xyz[2] < 0.0 ) {
        MgfEntityControl::doWarning("invalid color specification (negative CIE XYZ components) ... clipping to zero", context);
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

    Cie::transformColorFromXYZ2RGB(xyz, rgb);
    if ( Cie::clipGamut(rgb) ) {
        MgfEntityControl::doWarning("color desaturated during gamut clipping", context);
    }
    *colorOut = ColorRgb(rgb[0], rgb[1], rgb[2]);
}

void
MgfMaterialEntitySupport::specSamples(const ColorRgb &col, float *rgb) {
    rgb[0] = col.getR();
    rgb[1] = col.getG();
    rgb[2] = col.getB();
}

float
MgfMaterialEntitySupport::colorMax(const ColorRgb &col) {
    // We should check every wavelength in the visible spectrum, but
    // as a first approximation, only the three RGB primary colors
    // are checked
    float samples[NUMBER_OF_SAMPLES];

    MgfMaterialEntitySupport::specSamples(col, samples);

    float mx = -Numeric::HUGE_FLOAT_VALUE;
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
creates a new MATERIAL, which is added to the session material library.
The routine returns true if the material being used has changed
*/
bool
MgfMaterialEntitySupport::mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, ParseRuntimeContext *context) {
    ColorRgb Ed(0.0f, 0.0f, 0.0f);
    ColorRgb Es(0.0f, 0.0f, 0.0f);
    ColorRgb Rd(0.0f, 0.0f, 0.0f);
    ColorRgb Td(0.0f, 0.0f, 0.0f);
    ColorRgb Rs(0.0f, 0.0f, 0.0f);
    ColorRgb Ts(0.0f, 0.0f, 0.0f);
    ColorRgb A(0.0f, 0.0f, 0.0f);
    MaterialContext *currentMaterialContext = context->materialRepository.currentMaterialContext;
    const char *materialName = context->currentMaterialName;
    if ( !materialName || *materialName == '\0' ) {
        // This might cause strcmp to crash!
        materialName = "unnamed";
    }

    // Is it another material than the one used for the previous face ?? If not, the
    // material remains the same
    if ( strcmp(materialName, (*material)->getName()) == 0 && currentMaterialContext->clock == 0 ) {
        return false;
    }

    Material *storedMaterial = MgfMaterialEntitySupport::materialLookup(materialName, context);
    if ( storedMaterial != NULL && currentMaterialContext->clock == 0 ) {
        *material = storedMaterial;
        return true;
    }

    // New material, or a material that changed. Convert intensities and chromaticities
    // to our color model
    MgfMaterialEntitySupport::mgfGetColor(&currentMaterialContext->ed_c, currentMaterialContext->ed, &Ed, context);
    MgfMaterialEntitySupport::mgfGetColor(&currentMaterialContext->rd_c, currentMaterialContext->rd, &Rd, context);
    MgfMaterialEntitySupport::mgfGetColor(&currentMaterialContext->td_c, currentMaterialContext->td, &Td, context);
    MgfMaterialEntitySupport::mgfGetColor(&currentMaterialContext->rs_c, currentMaterialContext->rs, &Rs, context);
    MgfMaterialEntitySupport::mgfGetColor(&currentMaterialContext->ts_c, currentMaterialContext->ts, &Ts, context);

    // Check/correct range of reflectances and transmittances
    A = addColor(Rd, Rs);
    float a = MgfMaterialEntitySupport::colorMax(A);
    if ( a > 1.0f - Numeric::EPSILON_FLOAT ) {
        MgfEntityControl::doWarning("invalid material specification: total reflectance shall be < 1", context);
        a = (1.0f - Numeric::EPSILON_FLOAT) / a;
        Rd = scaleColor(Rd, a);
        Rs = scaleColor(Rs, a);
    }

    A = addColor(Td, Ts);
    a = MgfMaterialEntitySupport::colorMax(A);
    if ( a > 1.0f - Numeric::EPSILON_FLOAT ) {
        MgfEntityControl::doWarning("invalid material specification: total transmittance shall be < 1", context);
        a = (1.0f - Numeric::EPSILON_FLOAT) / a;
        Td = scaleColor(Td, a);
        Ts = scaleColor(Ts, a);
    }

    // Convert lumen / m^2 to W / m^2
    Ed = scaleColor(Ed, 1.0f / Cie::WHITE_EFFICACY);
    Es = ColorRgb(0.0f, 0.0f, 0.0f);

    float Nr;
    float Nt;

    // Specular power = (0.6/roughness)^2 (see mgf docs)
    if ( currentMaterialContext->rs_a != 0.0 ) {
        Nr = 0.6f / currentMaterialContext->rs_a;
        Nr *= Nr;
    } else {
        Nr = 0.0;
    }

    if ( currentMaterialContext->ts_a != 0.0 ) {
        Nt = 0.6f / currentMaterialContext->ts_a;
        Nt *= Nt;
    } else {
        Nt = 0.0;
    }

    if ( context->monochrome ) {
        const float EdGray = Cie::spectrumGray(Ed.getR(), Ed.getG(), Ed.getB());
        const float EsGray = Cie::spectrumGray(Es.getR(), Es.getG(), Es.getB());
        const float RdGray = Cie::spectrumGray(Rd.getR(), Rd.getG(), Rd.getB());
        const float RsGray = Cie::spectrumGray(Rs.getR(), Rs.getG(), Rs.getB());
        const float TdGray = Cie::spectrumGray(Td.getR(), Td.getG(), Td.getB());
        const float TsGray = Cie::spectrumGray(Ts.getR(), Ts.getG(), Ts.getB());
        Ed = ColorRgb(EdGray, EdGray, EdGray);
        Es = ColorRgb(EsGray, EsGray, EsGray);
        Rd = ColorRgb(RdGray, RdGray, RdGray);
        Rs = ColorRgb(RsGray, RsGray, RsGray);
        Td = ColorRgb(TdGray, TdGray, TdGray);
        Ts = ColorRgb(TsGray, TsGray, TsGray);
    }

    PhongEmitDistFunc* edf = NULL;
    if ( !isBlack(Ed) || !isBlack(Es) ) {
        CONSTEXPR float Ne = 0.0;
        edf = new PhongEmitDistFunc(&Ed, &Es, Ne);
    }

    PhongBidirReflDistFunc *brdf = NULL;
    if ( !isBlack(Rd) || !isBlack(Rs) ) {
        brdf = new PhongBidirReflDistFunc(&Rd, &Rs, Nr);
    }

    PhongBidirTransDistFunc *btdf = NULL;
    if ( !isBlack(Td) || !isBlack(Ts) ) {
        btdf = new PhongBidirTransDistFunc(
            &Td,
            &Ts,
            Nt,
            currentMaterialContext->nr,
            currentMaterialContext->ni);
    }

    PhongBidirScattDistFunc *bsdf = new PhongBidirScattDistFunc(brdf, btdf, NULL);

    (*material) = new Material(
        materialName,
         edf,
         bsdf,
        allSurfacesSided || currentMaterialContext->sided);

    context->materials->add((*material));

    // Reset the clock value to be aware of possible changes in future
    currentMaterialContext->clock = 0;

    return true;
}

void
MgfMaterialEntitySupport::initMaterialContextTables(ParseRuntimeContext *context) {
    context->materialRepository.reset();
    context->currentMaterialName = NULL;
}

/**
This routine returns true if the current material has changed
*/
bool
MgfMaterialEntitySupport::mgfMaterialChanged(const Material *material, const ParseRuntimeContext *context) {
    const char *materialName = context->currentMaterialName;
    if ( materialName == NULL || materialName[0] == '\0' ) {
        materialName = "unnamed";
    }

    // Is it another material than the one used for the previous face? If not, the
    // current material context remains the same
    if ( strcmp(materialName, material->getName()) == 0 &&
         context->materialRepository.currentMaterialContext->clock == 0 ) {
        return false;
    }

    return true;
}

/**
Handle material entity
*/
int
MgfMaterialEntitySupport::handleMaterialEntity(int ac, const char **av, ParseRuntimeContext *context) {
    int i;
    LookUpEntity<char *> *lp;
    MaterialContext *&currentMaterialContext = context->materialRepository.currentMaterialContext;
    LookUpTable<char *> *materialLookUpTable = context->materialRepository.materialLookUpTable;

    switch ( MgfEntityControl::mgfEntity(av[0], context) ) {

        case MGF_MATERIAL:
            // Get / set material context
            if ( ac > 4 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( ac == 1 ) {
                // Set unnamed material context
                context->materialRepository.unNamedMaterialContext = context->materialRepository.defaultMaterialContext;
                currentMaterialContext = &context->materialRepository.unNamedMaterialContext;
                context->currentMaterialName = NULL;
                return MGF_OK;
            }
            if ( !TokenValidationContext::isName(av[1]) ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            lp = materialLookUpTable->lookUpFind(av[1]);
            // Lookup context
            if ( lp == NULL ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentMaterialName = lp->key;
            currentMaterialContext = ((MaterialContext *)(lp->data));
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( currentMaterialContext == NULL) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( currentMaterialContext == NULL ) {
                // Create new material
                lp->key = new char[strlen(av[1]) + 1];
                if ( lp->key == NULL) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = new char[sizeof(MaterialContext)];
                if ( lp->data == NULL) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                context->currentMaterialName = lp->key;
                currentMaterialContext = ((MaterialContext *)(lp->data));
                currentMaterialContext->clock = 0;
            }
            i = currentMaterialContext->clock;
            if ( ac == 3 ) {
                // Use default template
                *currentMaterialContext = context->materialRepository.defaultMaterialContext;
                currentMaterialContext->clock = i + 1;
                return MGF_OK;
            }
            lp = materialLookUpTable->lookUpFind(av[3]);
            // Lookup template
            if ( lp == NULL ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == NULL ) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *currentMaterialContext = *((MaterialContext *)(lp->data));
            currentMaterialContext->clock = i + 1;
            return MGF_OK;

        case IR:
            // Set index of refraction
            if ( ac != 3 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->nr = strtof(av[1], NULL);
            currentMaterialContext->ni = strtof(av[2], NULL);
            if ( currentMaterialContext->nr <= Numeric::EPSILON ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            currentMaterialContext->clock++;
            return MGF_OK;

        case RD:
            // Set diffuse reflectance
            if ( ac != 2 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->rd = strtof(av[1], NULL);
            if ( currentMaterialContext->rd < 0. || currentMaterialContext->rd > 1.0 ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            currentMaterialContext->rd_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return MGF_OK;

        case ED:
            // Set diffuse emittance
            if ( ac != 2 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->ed = strtof(av[1], NULL);
            if ( currentMaterialContext->ed < 0.0 ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            currentMaterialContext->ed_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return MGF_OK;

        case TD:
            // Set diffuse transmittance
            if ( ac != 2 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->td = strtof(av[1], NULL);
            if ( currentMaterialContext->td < 0.0 || currentMaterialContext->td > 1.0 ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            currentMaterialContext->td_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return MGF_OK;

        case RS:
            // Set specular reflectance
            if ( ac != 3 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->rs = strtof(av[1], NULL);
            currentMaterialContext->rs_a = strtof(av[2], NULL);
            if ( currentMaterialContext->rs < 0.0 || currentMaterialContext->rs > 1.0 ||
                 currentMaterialContext->rs_a < 0.0 ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            currentMaterialContext->rs_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return MGF_OK;

        case TS:
            // Set specular transmittance
            if ( ac != 3 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->ts = strtof(av[1], NULL);
            currentMaterialContext->ts_a = strtof(av[2], NULL);
            if ( currentMaterialContext->ts < 0.0 || currentMaterialContext->ts > 1.0 ||
                 currentMaterialContext->ts_a < 0.0 ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            currentMaterialContext->ts_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return MGF_OK;

        case SIDES:
            // Set number of sides
            if ( ac != 2 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isInt(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            i = ((int)(strtol(av[1], NULL, 10)));
            if ( i == 1 ) {
                currentMaterialContext->sided = true;
            } else if ( i == 2 ) {
                    currentMaterialContext->sided = false;
                } else {
                    return MGF_ERRR_ILLGL_ARGMN_VAL;
                }
            currentMaterialContext->clock++;
            return MGF_OK;

        default:
            break;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}
