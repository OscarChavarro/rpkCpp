#include <cstring>
#include <cstdlib>

#include "common/dataStructures/LookUpEntity.h"
#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfHandlerColor.h"

/**
Handle color entity
*/
int
MgfHandlerColor::handleColorEntity(int ac, const char **av, ParseRuntimeContext *context) {
    int i;
    double wSum;
    LookUpEntity<char *> *lp;

    switch ( MgfDefinitions::mgfEntity(av[0], context) ) {
        case EntityTypeContext::COLOR:
            // Get/set color context
            if ( ac > 4 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed color context
                *(context->unNamedColorContext) = ColorContext::DEFAULT_COLOR_CONTEXT;
                context->currentColor = context->unNamedColorContext;
                return ParseErrorContext::MGF_OK;
            }
            if ( !TokenValidationContext::isName(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = context->colorRepository.colorLookUpTable->lookUpFind(av[1]); // Lookup context
            if ( lp == nullptr) {
                return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentColor = reinterpret_cast<ColorContext *>(lp->data);
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( context->currentColor == nullptr) {
                    return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return ParseErrorContext::MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( context->currentColor == nullptr) {    /* create new color context */
                lp->key = new char [strlen(av[1]) + 1];
                if ( lp->key == nullptr) {
                    return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = new char[sizeof(ColorContext)];
                if ( lp->data == nullptr) {
                    return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                context->currentColor = reinterpret_cast<ColorContext *>(lp->data);
                context->currentColor->clock = 0;
            }
            i = context->currentColor->clock;
            if ( ac == 3 ) {
                // Use default template
                *context->currentColor = ColorContext::DEFAULT_COLOR_CONTEXT;
                context->currentColor->clock = i + 1;
                return ParseErrorContext::MGF_OK;
            }
            lp = context->colorRepository.colorLookUpTable->lookUpFind(av[3]);
            // Lookup template
            if ( lp == nullptr) {
                return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *reinterpret_cast<ColorContext *>(lp->data);
            context->currentColor->clock = i + 1;
            return ParseErrorContext::MGF_OK;
        case EntityTypeContext::CXY:
            // Assign CIE XY value
            if ( ac != 3 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            context->currentColor->cx = strtof(av[1], nullptr);
            context->currentColor->cy = strtof(av[2], nullptr);
            context->currentColor->flags = COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG;
            if ( context->currentColor->cx < 0.0 || context->currentColor->cy < 0.0 ||
                 context->currentColor->cx + context->currentColor->cy > 1.0 ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            context->currentColor->clock++;
            return ParseErrorContext::MGF_OK;
        case EntityTypeContext::C_SPEC:
            // Assign spectral values
            if ( ac < 5 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setSpectrum(
                strtod(av[1], nullptr),
                strtod(av[2], nullptr),
                ac - 3,
                &av[3]);
        case EntityTypeContext::CCT:
            // Assign black body spectrum
            if ( ac != 2 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setBlackBodyTemperature(strtod(av[1], nullptr));
        case EntityTypeContext::C_MIX:
            // Mix colors
            if ( ac < 5 || (ac - 1) % 2 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            wSum = strtod(av[1], nullptr);
            lp = context->colorRepository.colorLookUpTable->lookUpFind(av[2]);
            if ( lp == nullptr ) {
                return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *reinterpret_cast<ColorContext *>(lp->data);
            for ( i = 3; i < ac; i += 2 ) {
                if ( !TokenValidationContext::isFloat(av[i]) ) {
                    return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
                }
                const double w = strtod(av[i], nullptr);
                lp = context->colorRepository.colorLookUpTable->lookUpFind(av[i + 1]);
                if ( lp == nullptr ) {
                    return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                if ( lp->data == nullptr ) {
                    return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                context->currentColor->mixColors(
                    wSum,
                    context->currentColor,
                    w,
                    reinterpret_cast<ColorContext *>(lp->data));
                wSum += w;
            }
            if ( wSum <= 0.0 ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            context->currentColor->clock++;
            return ParseErrorContext::MGF_OK;
        default:
            break;
    }
    return ParseErrorContext::MGF_ERROR_UNKNOWN_ENTITY;
}

/**
Empty context tables
*/
void
MgfHandlerColor::initColorContextTables(ParseRuntimeContext *context) {
    context->colorRepository.reset();
}
