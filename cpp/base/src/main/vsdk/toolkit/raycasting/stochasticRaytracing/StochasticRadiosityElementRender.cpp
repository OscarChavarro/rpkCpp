#include "vsdk/toolkit/material/RendererConfiguration.h"

/**
Rendering elements
*/


#ifdef RAYTRACING_ENABLED
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

ColorRgbMutable
StochasticRadiosityElement::stochasticRadiosityElementColor(const StochasticRadiosityElement *element) {
    ColorRgbMutable color{};

    switch ( StochasticRelaxation::activeState().show ) {
        case WhatToShow::SHOW_TOTAL_RADIANCE:
        case WhatToShow::SHOW_INDIRECT_RADIANCE:
            ToneMap::radianceToRgb(
                StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(element),
                &color,
                *StochasticRelaxation::activeState().toneMapOptions);
            break;
        case WhatToShow::SHOW_IMPORTANCE: {
            float gray;

            if ( element->importance > 1.0 ) {
                gray = 1.0F;
            } else {
                gray = element->importance < 0.0 ? 0.0F : element->importance;
            }

            color = ColorRgbMutable(gray, gray, gray);
            break;
        }
        default:
            Logger::fatal(
                -1,
                "stochasticRadiosityElementColor",
                "Don't know what to display (StochasticRelaxation::activeState().show = %d)",
                StochasticRelaxation::activeState().show);
    }

    return color;
}

ColorRgbMutable
StochasticRadiosityElement::vertexRadiance(const Vertex *v) {
    int count = 0;
    ColorRgbMutable radiance(0.0, 0.0, 0.0);

    radiance.clear();
    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *element = v->radianceData->get(i);
        if ( element->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *elem = static_cast<StochasticRadiosityElement *>(element);
        if ( !elem->regularSubElements ) {
            ColorRgbMutable elementRadiosity = StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(elem);
            radiance.add(radiance, elementRadiosity);
            count++;
        }
    }

    if ( count > 0 ) {
        radiance.scaleInverse(static_cast<float>(count), radiance);
    }

    return radiance;
}

/**
Same as above but for importance
*/
float
StochasticRadiosityElement::vertexImportance(const Vertex *v) {
    int count = 0;
    float imp = 0.0;

    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *element = static_cast<StochasticRadiosityElement *>(genericElement);
        if ( !element->regularSubElements ) {
            imp += element->importance;
            count++;
        }
    }

    if ( count > 0 ) {
        imp /= static_cast<float>(count);
    }

    return imp;
}

ColorRgbMutable
StochasticRadiosityElement::vertexColor(Vertex *v) {
    switch ( StochasticRelaxation::activeState().show ) {
        case WhatToShow::SHOW_TOTAL_RADIANCE:
        case WhatToShow::SHOW_INDIRECT_RADIANCE:
            ToneMap::radianceToRgb(
                vertexRadiance(v),
                &v->color,
                *StochasticRelaxation::activeState().toneMapOptions);
            break;
        case WhatToShow::SHOW_IMPORTANCE: {
            float gray = vertexImportance(v);
            if ( gray > 1.0 ) {
                gray = 1.0;
            }
            if ( gray < 0.0 ) {
                gray = 0.0;
            }
            v->color = ColorRgbMutable(gray, gray, gray);
            break;
        }
        default:
            Logger::fatal(-1, "vertexColor",
                     "Don't know what to display (StochasticRelaxation::activeState().show = %d)",
                     StochasticRelaxation::activeState().show);
    }

    return v->color;
}

/**
Compute new vertex colors
*/
void
StochasticRadiosityElement::stochasticRadiosityElementComputeNewVertexColors(Element *element) {
    const StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);
    vertexColor(stochasticRadiosityElement->vertices[0]);
    vertexColor(stochasticRadiosityElement->vertices[1]);
    vertexColor(stochasticRadiosityElement->vertices[2]);
    if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
        vertexColor(stochasticRadiosityElement->vertices[3]);
    }
}

void
StochasticRadiosityElement::stochasticRadiosityElementAdjustTVertexColors(Element *element) {
    const StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);
    Vertex *m[4];
    int i;
    int n;
    for ( i = 0, n = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
        m[i] = StochasticRadiosityElement::stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( n > 0 ) {
        ColorRgbMutable color = StochasticRadiosityElement::stochasticRadiosityElementColor(stochasticRadiosityElement);
        for ( i = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
            if ( m[i] ) {
                m[i]->color = ColorRgbMutable(
                    (m[i]->color.getR() + color.getR()) * 0.5,
                    (m[i]->color.getG() + color.getG()) * 0.5,
                    (m[i]->color.getB() + color.getB()) * 0.5);
            }
        }
    }
}

ColorRgbMutable
StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(const StochasticRadiosityElement *elem) {
    ColorRgbMutable radiance(0.0, 0.0, 0.0);
    radiance.subtract(elem->radiance[0], elem->sourceRad);

    if ( StochasticRelaxation::activeState().show != WhatToShow::SHOW_INDIRECT_RADIANCE ) {
        // sourceRad is self-emitted radiance when indirect-only is disabled.
        // Otherwise it represents direct illumination.
        radiance.add(radiance, elem->sourceRad);
        if ( StochasticRelaxation::activeState().indirectOnly || StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
            // Add self-emitted radiance
            radiance.add(radiance, elem->Ed);
        }
    }
    return radiance;
}

ColorRgbMutable
StochasticRadiosityElement::stochasticRadiosityElementDisplayRadianceAtPoint(const StochasticRadiosityElement *elem, double u, double v, const RendererConfiguration *renderOptions) {
    ColorRgbMutable radiance(0.0, 0.0, 0.0);
    if ( elem->basis->size == 1 ) {
        if ( renderOptions->isSmoothShading() ) {
            // Do Gouraud interpolation if required
            ColorRgbMutable rad[4];
            for ( int i = 0; i < elem->numberOfVertices; i++ ) {
                rad[i] = vertexRadiance(elem->vertices[i]);
            }
            switch ( elem->numberOfVertices ) {
                case 3:
                    radiance.interpolateBarycentric(rad[0], rad[1], rad[2], static_cast<float>(u), static_cast<float>(v));
                    break;
                case 4:
                    radiance.interpolateBiLinear(rad[0], rad[1], rad[2], rad[3], static_cast<float>(u), static_cast<float>(v));
                    break;
                default:
                    Logger::fatal(-1, "stochasticRadiosityElementDisplayRadianceAtPoint",
                             "can only handle triangular or quadrilateral elements");
            }
        } else {
            // Flat shading
            radiance = StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(elem);
        }
    } else {
        // Higher order approximations
        radiance = Basismcrad::colorAtUv(elem->basis, elem->radiance, u, v);
        if ( StochasticRelaxation::activeState().show == WhatToShow::SHOW_INDIRECT_RADIANCE ) {
            radiance.subtract(radiance, elem->sourceRad);
        }
    }
    return radiance;
}

#endif
