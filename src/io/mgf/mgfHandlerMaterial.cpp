#include "io/mgf/mgfHandlerMaterial.h"
#include "lookup.h"
#include "parser.h"

static MgfMaterialContext globalUnNamedMaterialContext = DEFAULT_MATERIAL;
static MgfMaterialContext globalDefaultMgfMaterial = DEFAULT_MATERIAL;
MgfMaterialContext *GLOBAL_mgf_currentMaterial = &globalUnNamedMaterialContext;

static LUTAB globalMaterialLookUpTable = LU_SINIT(free, free);

void
mgfClearMaterialTables() {
    globalUnNamedMaterialContext = globalDefaultMgfMaterial;
    GLOBAL_mgf_currentMaterial = &globalUnNamedMaterialContext;
    lookUpDone(&globalMaterialLookUpTable);
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
            // get/set material context
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
