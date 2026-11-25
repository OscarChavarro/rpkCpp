#include <cstring>
#include <cstdlib>

#include "java/lang/Float.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/util/ArrayList.txx"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/dataStructures/LookUpEntity.h"
#include "vsdk/toolkit/io/context/TokenValidationContext.h"
#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfMaterialEntitySupport.h"
#include "vsdk/toolkit/io/context/MaterialContext.h"

/**
Looks up a material with given name in the given material list. Returns
a pointer to the material if found, or nullptr if not found
*/
RpkMaterial *
MgfMaterialEntitySupport::materialLookup(const char *name, const ParseRuntimeContext *context) {
    for ( int i = 0; context->materials != nullptr && i < context->materials->size(); i++ ) {
        RpkMaterial *m = context->materials->get(i);
        if ( m != nullptr && m->getName() != nullptr && strcmp(m->getName(), name) == 0 ) {
            return m;
        }
    }
    return nullptr;
}

/**
Returns true if the color was desaturated during clipping against the
monitor gamut
*/
bool
MgfMaterialEntitySupport::clipGamut(double *rgb) {
    // Really SHOULD desaturate instead of just clipping!
    bool desaturated = false;
    for ( int i = 0; i < 3; i++ ) {
        if ( rgb[i] < 0.0 ) {
            rgb[i] = 0.0;
            desaturated = true;
        }
    }
    return desaturated;
}

/**
Translates mgf color into out color representation
*/
void
MgfMaterialEntitySupport::mgfGetColor(ColorContext *cin, double intensity, ColorRgb *colorOut, ParseRuntimeContext *context) {
    double xyz[3];
    double rgb[3];

    cin->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( cin->cy > VSDK::EPSILON ) {
        xyz[0] = cin->cx / cin->cy * intensity;
        xyz[1] = 1.0 * intensity;
        xyz[2] = (1.0 - cin->cx - cin->cy) / cin->cy * intensity;
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
        if ( xyz[1] < 0.0 ) {
            xyz[1] = 0.0;
        }
        if ( xyz[2] < 0.0 ) {
            xyz[2] = 0.0;
        }
    }

    context->parserConfig.getColorSpace().transformColorFromXYZ2RGB(xyz, rgb);
    if ( MgfMaterialEntitySupport::clipGamut(rgb) ) {
        MgfEntityControl::doWarning("color desaturated during gamut clipping", context);
    }
    *colorOut = ColorRgb(rgb[0], rgb[1], rgb[2]);
}

void
MgfMaterialEntitySupport::specSamples(const ColorRgb &col, double *rgb) {
    rgb[0] = col.getR();
    rgb[1] = col.getG();
    rgb[2] = col.getB();
}

double
MgfMaterialEntitySupport::colorMax(const ColorRgb &col) {
    // We should check every wavelength in the visible spectrum, but
    // as a first approximation, only the three RGB primary colors
    // are checked
    double samples[NUMBER_OF_SAMPLES];

    MgfMaterialEntitySupport::specSamples(col, samples);

    double mx = -java::Float::MAX_VALUE;
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
MgfMaterialEntitySupport::mgfGetCurrentMaterial(RpkMaterial **material, bool allSurfacesSided, ParseRuntimeContext *context) {
    ColorRgb Ed(0.0, 0.0, 0.0);
    ColorRgb Es(0.0, 0.0, 0.0);
    ColorRgb Rd(0.0, 0.0, 0.0);
    ColorRgb Td(0.0, 0.0, 0.0);
    ColorRgb Rs(0.0, 0.0, 0.0);
    ColorRgb Ts(0.0, 0.0, 0.0);
    ColorRgb A(0.0, 0.0, 0.0);
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

    RpkMaterial *storedMaterial = MgfMaterialEntitySupport::materialLookup(materialName, context);
    if ( storedMaterial != nullptr && currentMaterialContext->clock == 0 ) {
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
    A = Rd.add(Rs);
    double a = MgfMaterialEntitySupport::colorMax(A);
    if ( a > 1.0 - Numeric::EPSILON_FLOAT ) {
        MgfEntityControl::doWarning("invalid material specification: total reflectance shall be < 1", context);
        a = (1.0 - Numeric::EPSILON_FLOAT) / a;
        Rd = Rd.multiply(a);
        Rs = Rs.multiply(a);
    }

    A = Td.add(Ts);
    a = MgfMaterialEntitySupport::colorMax(A);
    if ( a > 1.0 - Numeric::EPSILON_FLOAT ) {
        MgfEntityControl::doWarning("invalid material specification: total transmittance shall be < 1", context);
        a = (1.0 - Numeric::EPSILON_FLOAT) / a;
        Td = Td.multiply(a);
        Ts = Ts.multiply(a);
    }

    // Convert lumen / m^2 to W / m^2
    Ed = Ed.multiply(1.0 / context->parserConfig.getColorSpace().getLuminousEfficacy());

    Es = ColorRgb(0.0, 0.0, 0.0);

    double Nr;
    double Nt;

    // Specular power = (0.6/roughness)^2 (see mgf docs)
    if ( currentMaterialContext->rs_a != 0.0 ) {
        Nr = 0.6 / currentMaterialContext->rs_a;
        Nr *= Nr;
    } else {
        Nr = 0.0;
    }

    if ( currentMaterialContext->ts_a != 0.0 ) {
        Nt = 0.6 / currentMaterialContext->ts_a;
        Nt *= Nt;
    } else {
        Nt = 0.0;
    }

    if ( context->monochrome ) {
        const Cie &colorSpace = context->parserConfig.getColorSpace();
        Ed = ColorRgb(colorSpace.spectrumGray(Ed.getR(), Ed.getG(), Ed.getB()), colorSpace.spectrumGray(Ed.getR(), Ed.getG(), Ed.getB()), colorSpace.spectrumGray(Ed.getR(), Ed.getG(), Ed.getB()));
        Es = ColorRgb(colorSpace.spectrumGray(Es.getR(), Es.getG(), Es.getB()), colorSpace.spectrumGray(Es.getR(), Es.getG(), Es.getB()), colorSpace.spectrumGray(Es.getR(), Es.getG(), Es.getB()));
        Rd = ColorRgb(colorSpace.spectrumGray(Rd.getR(), Rd.getG(), Rd.getB()), colorSpace.spectrumGray(Rd.getR(), Rd.getG(), Rd.getB()), colorSpace.spectrumGray(Rd.getR(), Rd.getG(), Rd.getB()));
        Rs = ColorRgb(colorSpace.spectrumGray(Rs.getR(), Rs.getG(), Rs.getB()), colorSpace.spectrumGray(Rs.getR(), Rs.getG(), Rs.getB()), colorSpace.spectrumGray(Rs.getR(), Rs.getG(), Rs.getB()));
        Td = ColorRgb(colorSpace.spectrumGray(Td.getR(), Td.getG(), Td.getB()), colorSpace.spectrumGray(Td.getR(), Td.getG(), Td.getB()), colorSpace.spectrumGray(Td.getR(), Td.getG(), Td.getB()));
        Ts = ColorRgb(colorSpace.spectrumGray(Ts.getR(), Ts.getG(), Ts.getB()), colorSpace.spectrumGray(Ts.getR(), Ts.getG(), Ts.getB()), colorSpace.spectrumGray(Ts.getR(), Ts.getG(), Ts.getB()));
    }

    // The lobes keep their exponents in single precision
    const PhongLobe *emission = nullptr;
    if ( !Ed.isBlack() || !Es.isBlack() ) {
        constexpr double Ne = 0.0;
        emission = new PhongLobe(Ed, Es, static_cast<float>(Ne));
    }

    const PhongLobe *reflection = nullptr;
    if ( !Rd.isBlack() || !Rs.isBlack() ) {
        reflection = new PhongLobe(Rd, Rs, static_cast<float>(Nr));
    }

    const PhongLobe *transmission = nullptr;
    float refractionIndexReal = 1.0F;
    float refractionIndexImaginary = 0.0F;
    if ( !Td.isBlack() || !Ts.isBlack() ) {
        transmission = new PhongLobe(Td, Ts, static_cast<float>(Nt));
        refractionIndexReal = static_cast<float>(currentMaterialContext->nr);
        refractionIndexImaginary = static_cast<float>(currentMaterialContext->ni);
    }

    (*material) = new RpkMaterial(
        materialName,
        emission,
        true,
        reflection,
        transmission,
        nullptr,
        refractionIndexReal,
        refractionIndexImaginary,
        allSurfacesSided || currentMaterialContext->sided);

    context->materials->add((*material));

    // Reset the clock value to be aware of possible changes in future
    currentMaterialContext->clock = 0;

    return true;
}

void
MgfMaterialEntitySupport::initMaterialContextTables(ParseRuntimeContext *context) {
    context->materialRepository.reset();
    context->currentMaterialName = nullptr;
}

/**
This routine returns true if the current material has changed
*/
bool
MgfMaterialEntitySupport::mgfMaterialChanged(const RpkMaterial *material, const ParseRuntimeContext *context) {
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
MgfMaterialEntitySupport::handleMaterialEntity(int ac, const char **av, ParseRuntimeContext *context) {
    int i;
    LookUpEntity<char *> *lp;
    MaterialContext *&currentMaterialContext = context->materialRepository.currentMaterialContext;
    LookUpTable<char *> *materialLookUpTable = context->materialRepository.materialLookUpTable;

    switch ( MgfEntityControl::mgfEntity(av[0], context) ) {

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
            if ( currentMaterialContext->nr <= VSDK::EPSILON ) {
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
