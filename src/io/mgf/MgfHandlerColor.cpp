#include <cstring>

#include "io/context/LookUpEntity.h"
#include "io/context/WordsContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfHandlerColor.h"

static LookUpTable globalColorTable(LookUpBehaviors::owningCString());

/**
Handle color entity
*/
int
handleColorEntity(int ac, const char **av, MgfParseSession *context) {
    int i;
    double wSum;
    LookUpEntity *lp;

    switch ( mgfEntity(av[0], context) ) {
        case EntityContext::COLOR:
            // Get/set color context
            if ( ac > 4 ) {
                return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed color context
                *(context->unNamedColorContext) = DEFAULT_COLOR_CONTEXT;
                context->currentColor = context->unNamedColorContext;
                return ErrorCodeContext::MGF_OK;
            }
            if ( !WordsContext::isName(av[1]) ) {
                return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = globalColorTable.lookUpFind(av[1]); // Lookup context
            if ( lp == nullptr) {
                return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentColor = reinterpret_cast<ColorContext *>(lp->data);
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( context->currentColor == nullptr) {
                    return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return ErrorCodeContext::MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( context->currentColor == nullptr) {    /* create new color context */
                lp->key = new char [strlen(av[1]) + 1];
                if ( lp->key == nullptr) {
                    return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = new char[sizeof(ColorContext)];
                if ( lp->data == nullptr) {
                    return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                context->currentColor = reinterpret_cast<ColorContext *>(lp->data);
                context->currentColor->clock = 0;
            }
            i = context->currentColor->clock;
            if ( ac == 3 ) {
                // Use default template
                *context->currentColor = DEFAULT_COLOR_CONTEXT;
                context->currentColor->clock = i + 1;
                return ErrorCodeContext::MGF_OK;
            }
            lp = globalColorTable.lookUpFind(av[3]);
            // Lookup template
            if ( lp == nullptr) {
                return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *reinterpret_cast<ColorContext *>(lp->data);
            context->currentColor->clock = i + 1;
            return ErrorCodeContext::MGF_OK;
        case EntityContext::CXY:
            // Assign CIE XY value
            if ( ac != 3 ) {
                return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !WordsContext::isFloat(av[1]) || !WordsContext::isFloat(av[2]) ) {
                return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            context->currentColor->cx = strtof(av[1], nullptr);
            context->currentColor->cy = strtof(av[2], nullptr);
            context->currentColor->flags = COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG;
            if ( context->currentColor->cx < 0.0 || context->currentColor->cy < 0.0 ||
                 context->currentColor->cx + context->currentColor->cy > 1.0 ) {
                return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            context->currentColor->clock++;
            return ErrorCodeContext::MGF_OK;
        case EntityContext::C_SPEC:
            // Assign spectral values
            if ( ac < 5 ) {
                return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !WordsContext::isFloat(av[1]) || !WordsContext::isFloat(av[2]) ) {
                return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setSpectrum(
                strtod(av[1], nullptr),
                strtod(av[2], nullptr),
                ac - 3,
                &av[3]);
        case EntityContext::CCT:
            // Assign black body spectrum
            if ( ac != 2 ) {
                return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !WordsContext::isFloat(av[1]) ) {
                return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            return context->currentColor->setBlackBodyTemperature(strtod(av[1], nullptr));
        case EntityContext::C_MIX:
            // Mix colors
            if ( ac < 5 || (ac - 1) % 2 ) {
                return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !WordsContext::isFloat(av[1]) ) {
                return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            wSum = strtod(av[1], nullptr);
            lp = globalColorTable.lookUpFind(av[2]);
            if ( lp == nullptr ) {
                return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *context->currentColor = *reinterpret_cast<ColorContext *>(lp->data);
            for ( i = 3; i < ac; i += 2 ) {
                if ( !WordsContext::isFloat(av[i]) ) {
                    return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
                }
                const double w = strtod(av[i], nullptr);
                lp = globalColorTable.lookUpFind(av[i + 1]);
                if ( lp == nullptr ) {
                    return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                if ( lp->data == nullptr ) {
                    return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                context->currentColor->mixColors(
                    wSum,
                    context->currentColor,
                    w,
                    reinterpret_cast<ColorContext *>(lp->data));
                wSum += w;
            }
            if ( wSum <= 0.0 ) {
                return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            context->currentColor->clock++;
            return ErrorCodeContext::MGF_OK;
        default:
            break;
    }
    return ErrorCodeContext::MGF_ERROR_UNKNOWN_ENTITY;
}

/**
Empty context tables
*/
void
initColorContextTables(MgfParseSession *context) {
    *(context->unNamedColorContext) = DEFAULT_COLOR_CONTEXT;
    context->currentColor = context->unNamedColorContext;
    globalColorTable.lookUpDone();
}
