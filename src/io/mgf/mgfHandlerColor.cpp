#include <cstring>

#include "io/mgf/words.h"
#include "io/mgf/mgfHandlerColor.h"
#include "io/mgf/mgfDefinitions.h"

// Color lookup table
static LookUpTable globalColorTable = LOOK_UP_INIT(lookUpRemove, lookUpRemove);

/**
Handle color entity
*/
int
handleColorEntity(int ac, const char **av, MgfContext *context) {
    int i;
    double w;
    double wSum;
    LookUpEntity *lp;

    switch ( mgfEntity(av[0], context) ) {
        case MgfEntity::COLOR:
            // Get/set color context
            if ( ac > 4 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed color context
                *(context->unNamedColorContext) = DEFAULT_COLOR_CONTEXT;
                context->currentColor = context->unNamedColorContext;
                return MgfErrorCode::MGF_OK;
            }
            if ( !isNameWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lookUpFind(&globalColorTable, av[1]); // Lookup context
            if ( lp == nullptr) {
                return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentColor = (MgfColorContext *) lp->data;
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( context->currentColor == nullptr) {
                    return MgfErrorCode::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MgfErrorCode::MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( context->currentColor == nullptr) {    /* create new color context */
                lp->key = new char [strlen(av[1]) + 1];
                if ( lp->key == nullptr) {
                    return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = new char[sizeof(MgfColorContext)];
                if ( lp->data == nullptr) {
                    return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
                }
                context->currentColor = (MgfColorContext *) lp->data;
                context->currentColor->clock = 0;
            }
            i = context->currentColor->clock;
            if ( ac == 3 ) {
                // Use default template
                *context->currentColor = DEFAULT_COLOR_CONTEXT;
                context->currentColor->clock = i + 1;
                return MgfErrorCode::MGF_OK;
            }
            lp = lookUpFind(&globalColorTable, av[3]);
            // Lookup template
            if ( lp == nullptr) {
                return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MgfErrorCode::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *(MgfColorContext *) lp->data;
            context->currentColor->clock = i + 1;
            return MgfErrorCode::MGF_OK;
        case MgfEntity::CXY:
            // Assign CIE XY value
            if ( ac != 3 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            context->currentColor->cx = strtof(av[1], nullptr);
            context->currentColor->cy = strtof(av[2], nullptr);
            context->currentColor->flags = COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG;
            if ( context->currentColor->cx < 0.0 || context->currentColor->cy < 0.0 ||
                 context->currentColor->cx + context->currentColor->cy > 1.0 ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            context->currentColor->clock++;
            return MgfErrorCode::MGF_OK;
        case MgfEntity::C_SPEC:
            // Assign spectral values
            if ( ac < 5 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setSpectrum(
                strtod(av[1], nullptr),
                strtod(av[2], nullptr),
                ac - 3,
                av + 3);
        case MgfEntity::CCT:
            // Assign black body spectrum
            if ( ac != 2 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setBlackBodyTemperature(strtod(av[1], nullptr));
        case MgfEntity::C_MIX:
            // Mix colors
            if ( ac < 5 || (ac - 1) % 2 ) {
                return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
            }
            wSum = strtod(av[1], nullptr);
            lp = lookUpFind(&globalColorTable, av[2]);
            if ( lp == nullptr ) {
                return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MgfErrorCode::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *(MgfColorContext *) lp->data;
            for ( i = 3; i < ac; i += 2 ) {
                if ( !isFloatWords(av[i]) ) {
                    return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
                }
                w = strtod(av[i], nullptr);
                lp = lookUpFind(&globalColorTable, av[i + 1]);
                if ( lp == nullptr ) {
                    return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
                }
                if ( lp->data == nullptr ) {
                    return MgfErrorCode::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                context->currentColor->mixColors(
                    wSum,
                    context->currentColor,
                    w,
                    (MgfColorContext *) lp->data);
                wSum += w;
            }
            if ( wSum <= 0.0 ) {
                return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            context->currentColor->clock++;
            return MgfErrorCode::MGF_OK;
        default:
            break;
    }
    return MgfErrorCode::MGF_ERROR_UNKNOWN_ENTITY;
}

/**
Empty context tables
*/
void
initColorContextTables(MgfContext *context) {
    *(context->unNamedColorContext) = DEFAULT_COLOR_CONTEXT;
    context->currentColor = context->unNamedColorContext;
    lookUpDone(&globalColorTable);
}
