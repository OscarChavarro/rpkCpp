#include <cstring>
#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/dataStructures/LookUpEntity.h"
#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfHandlerMaterial.h"
#include "io/context/MaterialContext.h"

/**
Looks up a material with given name in the given material list. Returns
a pointer to the material if found, or nullptr if not found
*/
Material *
MgfHandlerMaterial::materialLookup(const char *name, const ParseRuntimeContext *context) {
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
void
MgfHandlerMaterial::mgfGetColor(ColorContext *cin, float intensity, ColorRgb *colorOut, ParseRuntimeContext *context) {
    float xyz[3];
    float rgb[3];

    cin->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( cin->cy > Numeric::EPSILON ) {
        xyz[0] = cin->cx / cin->cy * intensity;
        xyz[1] = 1.0f * intensity;
        xyz[2] = (1.0f - cin->cx - cin->cy) / cin->cy * intensity;
    } else {
        MgfDefinitions::doWarning("invalid color specification (Y<=0) ... setting to black", context);
        xyz[0] = 0.0;
        xyz[1] = 0.0;
        xyz[2] = 0.0;
    }

    if ( xyz[0] < 0.0 || xyz[1] < 0.0 || xyz[2] < 0.0 ) {
        MgfDefinitions::doWarning("invalid color specification (negative CIE XYZ components) ... clipping to zero", context);
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
        MgfDefinitions::doWarning("color desaturated during gamut clipping", context);
    }
    colorOut->set(rgb[0], rgb[1], rgb[2]);
}

void
MgfHandlerMaterial::specSamples(const ColorRgb &col, float *rgb) {
    rgb[0] = col.r;
    rgb[1] = col.g;
    rgb[2] = col.b;
}

float
MgfHandlerMaterial::colorMax(ColorRgb col) {
    // We should check every wavelength in the visible spectrum, but
    // as a first approximation, only the three RGB primary colors
    // are checked
    float samples[NUMBER_OF_SAMPLES];

    MgfHandlerMaterial::specSamples(col, samples);

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
MgfHandlerMaterial::mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, ParseRuntimeContext *context) {
    ColorRgb Ed;
    ColorRgb Es;
    ColorRgb Rd;
    ColorRgb Td;
    ColorRgb Rs;
    ColorRgb Ts;
    ColorRgb A;
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

    Material *storedMaterial = MgfHandlerMaterial::materialLookup(materialName, context);
    if ( storedMaterial != nullptr && currentMaterialContext->clock == 0 ) {
        *material = storedMaterial;
        return true;
    }

    // New material, or a material that changed. Convert intensities and chromaticities
    // to our color model
    MgfHandlerMaterial::mgfGetColor(&currentMaterialContext->ed_c, currentMaterialContext->ed, &Ed, context);
    MgfHandlerMaterial::mgfGetColor(&currentMaterialContext->rd_c, currentMaterialContext->rd, &Rd, context);
    MgfHandlerMaterial::mgfGetColor(&currentMaterialContext->td_c, currentMaterialContext->td, &Td, context);
    MgfHandlerMaterial::mgfGetColor(&currentMaterialContext->rs_c, currentMaterialContext->rs, &Rs, context);
    MgfHandlerMaterial::mgfGetColor(&currentMaterialContext->ts_c, currentMaterialContext->ts, &Ts, context);

    // Check/correct range of reflectances and transmittances
    A.add(Rd, Rs);
    float a = MgfHandlerMaterial::colorMax(A);
    if ( a > 1.0f - Numeric::EPSILON_FLOAT ) {
        MgfDefinitions::doWarning("invalid material specification: total reflectance shall be < 1", context);
        a = (1.0f - Numeric::EPSILON_FLOAT) / a;
        Rd.scale(a);
        Rs.scale(a);
    }

    A.add(Td, Ts);
    a = MgfHandlerMaterial::colorMax(A);
    if ( a > 1.0f - Numeric::EPSILON_FLOAT ) {
        MgfDefinitions::doWarning("invalid material specification: total transmittance shall be < 1", context);
        a = (1.0f - Numeric::EPSILON_FLOAT) / a;
        Td.scale(a);
        Ts.scale(a);
    }

    // Convert lumen / m^2 to W / m^2
    Ed.scale(1.0f / Cie::WHITE_EFFICACY);

    Es.clear();

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
        Ed.setMonochrome(Ed.gray());
        Es.setMonochrome(Es.gray());
        Rd.setMonochrome(Rd.gray());
        Rs.setMonochrome(Rs.gray());
        Td.setMonochrome(Td.gray());
        Ts.setMonochrome(Ts.gray());
    }

    PhongEmittanceDistributionFunction* edf = nullptr;
    if ( !Ed.isBlack() || !Es.isBlack() ) {
        constexpr float Ne = 0.0;
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
            currentMaterialContext->nr,
            currentMaterialContext->ni);
    }

    PhongBidirectionalScatteringDistributionFunction *bsdf = new PhongBidirectionalScatteringDistributionFunction(brdf, btdf, nullptr);

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
MgfHandlerMaterial::initMaterialContextTables(ParseRuntimeContext *context) {
    context->materialRepository.reset();
    context->currentMaterialName = nullptr;
}

/**
This routine returns true if the current material has changed
*/
bool
MgfHandlerMaterial::mgfMaterialChanged(const Material *material, const ParseRuntimeContext *context) {
    const char *materialName = context->currentMaterialName;
    if ( materialName == nullptr || materialName[0] == '\0' ) {
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
MgfHandlerMaterial::handleMaterialEntity(int ac, const char **av, ParseRuntimeContext *context) {
    int i;
    LookUpEntity<char *> *lp;
    MaterialContext *&currentMaterialContext = context->materialRepository.currentMaterialContext;
    LookUpTable<char *> *materialLookUpTable = context->materialRepository.materialLookUpTable;

    switch ( MgfDefinitions::mgfEntity(av[0], context) ) {

        case EntityTypeContext::MGF_MATERIAL:
            // Get / set material context
            if ( ac > 4 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed material context
                context->materialRepository.unNamedMaterialContext = context->materialRepository.defaultMaterialContext;
                currentMaterialContext = &context->materialRepository.unNamedMaterialContext;
                context->currentMaterialName = nullptr;
                return ParseErrorContext::MGF_OK;
            }
            if ( !TokenValidationContext::isName(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = materialLookUpTable->lookUpFind(av[1]);
            // Lookup context
            if ( lp == nullptr ) {
                return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentMaterialName = lp->key;
            currentMaterialContext = reinterpret_cast<MaterialContext *>(lp->data);
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( currentMaterialContext == nullptr) {
                    return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return ParseErrorContext::MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( currentMaterialContext == nullptr ) {
                // Create new material
                lp->key = new char[strlen(av[1]) + 1];
                if ( lp->key == nullptr) {
                    return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = new char[sizeof(MaterialContext)];
                if ( lp->data == nullptr) {
                    return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                context->currentMaterialName = lp->key;
                currentMaterialContext = reinterpret_cast<MaterialContext *>(lp->data);
                currentMaterialContext->clock = 0;
            }
            i = currentMaterialContext->clock;
            if ( ac == 3 ) {
                // Use default template
                *currentMaterialContext = context->materialRepository.defaultMaterialContext;
                currentMaterialContext->clock = i + 1;
                return ParseErrorContext::MGF_OK;
            }
            lp = materialLookUpTable->lookUpFind(av[3]);
            // Lookup template
            if ( lp == nullptr ) {
                return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr ) {
                return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *currentMaterialContext = *reinterpret_cast<MaterialContext *>(lp->data);
            currentMaterialContext->clock = i + 1;
            return ParseErrorContext::MGF_OK;

        case EntityTypeContext::IR:
            // Set index of refraction
            if ( ac != 3 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->nr = strtof(av[1], nullptr);
            currentMaterialContext->ni = strtof(av[2], nullptr);
            if ( currentMaterialContext->nr <= Numeric::EPSILON ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            currentMaterialContext->clock++;
            return ParseErrorContext::MGF_OK;

        case EntityTypeContext::RD:
            // Set diffuse reflectance
            if ( ac != 2 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->rd = strtof(av[1], nullptr);
            if ( currentMaterialContext->rd < 0. || currentMaterialContext->rd > 1.0 ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            currentMaterialContext->rd_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return ParseErrorContext::MGF_OK;

        case EntityTypeContext::ED:
            // Set diffuse emittance
            if ( ac != 2 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->ed = strtof(av[1], nullptr);
            if ( currentMaterialContext->ed < 0.0 ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            currentMaterialContext->ed_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return ParseErrorContext::MGF_OK;

        case EntityTypeContext::TD:
            // Set diffuse transmittance
            if ( ac != 2 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->td = strtof(av[1], nullptr);
            if ( currentMaterialContext->td < 0.0 || currentMaterialContext->td > 1.0 ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            currentMaterialContext->td_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return ParseErrorContext::MGF_OK;

        case EntityTypeContext::RS:
            // Set specular reflectance
            if ( ac != 3 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->rs = strtof(av[1], nullptr);
            currentMaterialContext->rs_a = strtof(av[2], nullptr);
            if ( currentMaterialContext->rs < 0.0 || currentMaterialContext->rs > 1.0 ||
                 currentMaterialContext->rs_a < 0.0 ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            currentMaterialContext->rs_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return ParseErrorContext::MGF_OK;

        case EntityTypeContext::TS:
            // Set specular transmittance
            if ( ac != 3 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentMaterialContext->ts = strtof(av[1], nullptr);
            currentMaterialContext->ts_a = strtof(av[2], nullptr);
            if ( currentMaterialContext->ts < 0.0 || currentMaterialContext->ts > 1.0 ||
                 currentMaterialContext->ts_a < 0.0 ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            currentMaterialContext->ts_c = *(context->currentColor);
            currentMaterialContext->clock++;
            return ParseErrorContext::MGF_OK;

        case EntityTypeContext::SIDES:
            // Set number of sides
            if ( ac != 2 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isInt(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            i = static_cast<int>(strtol(av[1], nullptr, 10));
            if ( i == 1 ) {
                currentMaterialContext->sided = true;
            } else if ( i == 2 ) {
                    currentMaterialContext->sided = false;
                } else {
                    return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
            currentMaterialContext->clock++;
            return ParseErrorContext::MGF_OK;

        default:
            break;
    }
    return ParseErrorContext::MGF_ERROR_UNKNOWN_ENTITY;
}
