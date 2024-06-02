#include "java/util/ArrayList.txx"
#include "GALERKIN/processing/GalerkingElementDebug.h"

void
GalerkingElementDebug::printGalerkinElementHierarchy(const GalerkinElement *galerkinElement, const int level) {
    if ( level == 0 ) {
        printf("= GalerkinElement hierarchy ================================================================\n");
    }
    switch ( level ) {
        case 0:
            break;
        case 1:
            printf("* ");
            break;
        case 2:
            printf("  - ");
            break;
        case 3:
            printf("    . ");
            break;
        default:
            printf("   ");
            for ( int j = 0; j < level; j++ ) {
                printf(" ");
            }
            printf("[%d] ", level);
            break;
    }
    if ( galerkinElement != nullptr ) {
        printf("%d ( ", galerkinElement->id);
        if ( galerkinElement->geometry != nullptr ) {
            switch ( galerkinElement->geometry->className ) {
                case GeometryClassId::PATCH_SET:
                    printf("geom patchSet");
                    break;
                case GeometryClassId::SURFACE_MESH:
                    printf("geom mesh");
                    break;
                case GeometryClassId::COMPOUND:
                    printf("geom compound");
                    break;
                default:
                    break;
            }
            printf(" %d ", galerkinElement->geometry->id);
        }
        if ( galerkinElement->patch != nullptr ) {
            printf("patch %d ", galerkinElement->patch->id);
        }

        printf(")%s",
           galerkinElement->flags & ElementFlags::IS_LIGHT_SOURCE_MASK ? " [isLight]" : "");

        if ( galerkinElement->interactions->size() > 0 ) {
            printf(" interactions: %ld\n", galerkinElement->interactions->size());
            printf("    --->");
            for ( int i = 0; i < galerkinElement->interactions->size(); i++ ) {
                const Interaction *interaction = galerkinElement->interactions->get(i);
                printf("[%d->%d] ", interaction->sourceElement->id, interaction->receiverElement->id);
            }
        }

        printf("\n");

        for ( int i = 0;
              galerkinElement->irregularSubElements != nullptr && i < galerkinElement->irregularSubElements->size();
              i++ ) {
            printGalerkinElementHierarchy(
                    (const GalerkinElement *) galerkinElement->irregularSubElements->get(i), level + 1);
        }
    } else {
        printf("NULL\n");
    }
}
