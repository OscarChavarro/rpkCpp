#include <string.h>
#include <stdlib.h>

#include "common/dataStructures/LookUpEntity.h"
#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfColorEntitySupport.h"

/**
Handle color entity
*/
int
MgfColorEntitySupport::handleColorEntity(int ac, const char **av, ParseRuntimeContext *context) {
    int i;
    double wSum;
    LookUpEntity<char *> *lp;

    switch ( MgfEntityControl::mgfEntity(av[0], context) ) {
        case COLOR:
            // Get/set color context
            if ( ac > 4 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( ac == 1 ) {
                // Set unnamed color context
                *(context->unNamedColorContext) = ColorContext::DEFAULT_COLOR_CONTEXT;
                context->currentColor = context->unNamedColorContext;
                return MGF_OK;
            }
            if ( !TokenValidationContext::isName(av[1]) ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            lp = context->colorRepository.colorLookUpTable->lookUpFind(av[1]); // Lookup context
            if ( lp == NULL) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentColor = ((ColorContext *)(lp->data));
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( context->currentColor == NULL) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( context->currentColor == NULL) {    /* create new color context */
                lp->key = new char [strlen(av[1]) + 1];
                if ( lp->key == NULL) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = new char[sizeof(ColorContext)];
                if ( lp->data == NULL) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                context->currentColor = ((ColorContext *)(lp->data));
                context->currentColor->clock = 0;
            }
            i = context->currentColor->clock;
            if ( ac == 3 ) {
                // Use default template
                *context->currentColor = ColorContext::DEFAULT_COLOR_CONTEXT;
                context->currentColor->clock = i + 1;
                return MGF_OK;
            }
            lp = context->colorRepository.colorLookUpTable->lookUpFind(av[3]);
            // Lookup template
            if ( lp == NULL) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == NULL) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *((ColorContext *)(lp->data));
            context->currentColor->clock = i + 1;
            return MGF_OK;
        case CXY:
            // Assign CIE XY value
            if ( ac != 3 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            context->currentColor->cx = strtof(av[1], NULL);
            context->currentColor->cy = strtof(av[2], NULL);
            context->currentColor->flags = COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG;
            if ( context->currentColor->cx < 0.0 || context->currentColor->cy < 0.0 ||
                 context->currentColor->cx + context->currentColor->cy > 1.0 ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            context->currentColor->clock++;
            return MGF_OK;
        case C_SPEC:
            // Assign spectral values
            if ( ac < 5 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setSpectrum(
                strtod(av[1], NULL),
                strtod(av[2], NULL),
                ac - 3,
                &av[3]);
        case CCT:
            // Assign black body spectrum
            if ( ac != 2 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setBlackBodyTemperature(strtod(av[1], NULL));
        case C_MIX:
            // Mix colors
            if ( ac < 5 || (ac - 1) % 2 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( !TokenValidationContext::isFloat(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            wSum = strtod(av[1], NULL);
            lp = context->colorRepository.colorLookUpTable->lookUpFind(av[2]);
            if ( lp == NULL ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == NULL) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *((ColorContext *)(lp->data));
            for ( i = 3; i < ac; i += 2 ) {
                if ( !TokenValidationContext::isFloat(av[i]) ) {
                    return MGF_ERROR_ARGUMENT_TYPE;
                }
                const double w = strtod(av[i], NULL);
                lp = context->colorRepository.colorLookUpTable->lookUpFind(av[i + 1]);
                if ( lp == NULL ) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                if ( lp->data == NULL ) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                context->currentColor->mixColors(
                    wSum,
                    context->currentColor,
                    w,
                    ((ColorContext *)(lp->data)));
                wSum += w;
            }
            if ( wSum <= 0.0 ) {
                return MGF_ERRR_ILLGL_ARGMN_VAL;
            }
            context->currentColor->clock++;
            return MGF_OK;
        default:
            break;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}

/**
Empty context tables
*/
void
MgfColorEntitySupport::initColorContextTables(ParseRuntimeContext *context) {
    context->colorRepository.reset();
}
