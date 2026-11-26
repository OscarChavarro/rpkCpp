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

ColorRgb
StochasticRadiosityElement::stochasticRadiosityElementColor(const StochasticRadiosityElement *element) {
    ColorRgb color{};

    switch ( StochasticRelaxation::activeState().show ) {
        case WhatToShow::SHOW_TOTAL_RADIANCE:
        case WhatToShow::SHOW_INDIRECT_RADIANCE:
            color = ToneMap::radianceToRgb(
                StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(element),
                *StochasticRelaxation::activeState().toneMapOptions);
            break;
        case WhatToShow::SHOW_IMPORTANCE: {
            double gray;

            if ( element->importance > 1.0 ) {
                gray = 1.0;
            } else {
                gray = element->importance < 0.0 ? 0.0 : element->importance;
            }

            color = ColorRgb(gray, gray, gray);
            break;
        }
        default:
            Logger::reportMessage("StochasticRadiosityElementRender", Logger::FATAL_ERROR, "stochasticRadiosityElementColor", java::String::format("Don't know what to display (StochasticRelaxation::activeState().show = %d)", StochasticRelaxation::activeState().show));
    }

    return color;
}

ColorRgb
StochasticRadiosityElement::vertexRadiance(const RpkVertex *v) {
    int count = 0;
    double radiance[3] = {0.0, 0.0, 0.0};

    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *element = v->radianceData->get(i);
        if ( element->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *elem = static_cast<StochasticRadiosityElement *>(element);
        if ( !elem->regularSubElements ) {
            ColorRgb elementRadiosity = StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(elem);
            radiance[0] += elementRadiosity.getR();
            radiance[1] += elementRadiosity.getG();
            radiance[2] += elementRadiosity.getB();
            count++;
        }
    }

    if ( count > 0 ) {
        const double scale = 1.0 / static_cast<double>(count);
        radiance[0] = scale * radiance[0];
        radiance[1] = scale * radiance[1];
        radiance[2] = scale * radiance[2];
    }

    return ColorRgb(radiance[0], radiance[1], radiance[2]);
}

/**
Same as above but for importance
*/
double
StochasticRadiosityElement::vertexImportance(const RpkVertex *v) {
    int count = 0;
    double imp = 0.0;

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
        imp /= static_cast<double>(count);
    }

    return imp;
}

ColorRgb
StochasticRadiosityElement::vertexColor(RpkVertex *v) {
    switch ( StochasticRelaxation::activeState().show ) {
        case WhatToShow::SHOW_TOTAL_RADIANCE:
        case WhatToShow::SHOW_INDIRECT_RADIANCE:
            v->color = ToneMap::radianceToRgb(
                vertexRadiance(v),
                *StochasticRelaxation::activeState().toneMapOptions);
            break;
        case WhatToShow::SHOW_IMPORTANCE: {
            double gray = vertexImportance(v);
            if ( gray > 1.0 ) {
                gray = 1.0;
            }
            if ( gray < 0.0 ) {
                gray = 0.0;
            }
            v->color = ColorRgb(gray, gray, gray);
            break;
        }
        default:
            Logger::reportMessage("StochasticRadiosityElementRender", Logger::FATAL_ERROR, "vertexColor", java::String::format("Don't know what to display (StochasticRelaxation::activeState().show = %d)", StochasticRelaxation::activeState().show));
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
    RpkVertex *m[4];
    int i;
    int n;
    for ( i = 0, n = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
        m[i] = StochasticRadiosityElement::stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( n > 0 ) {
        ColorRgb color = StochasticRadiosityElement::stochasticRadiosityElementColor(stochasticRadiosityElement);
        for ( i = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
            if ( m[i] ) {
                m[i]->color = ColorRgb(
                    (m[i]->color.getR() + color.getR()) * 0.5,
                    (m[i]->color.getG() + color.getG()) * 0.5,
                    (m[i]->color.getB() + color.getB()) * 0.5);
            }
        }
    }
}

ColorRgb
StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(const StochasticRadiosityElement *elem) {
    double radiance[3] = {
        elem->radiance[0].getR() - elem->sourceRad.getR(),
        elem->radiance[0].getG() - elem->sourceRad.getG(),
        elem->radiance[0].getB() - elem->sourceRad.getB()
    };

    if ( StochasticRelaxation::activeState().show != WhatToShow::SHOW_INDIRECT_RADIANCE ) {
        // sourceRad is self-emitted radiance when indirect-only is disabled.
        // Otherwise it represents direct illumination.
        radiance[0] += elem->sourceRad.getR();
        radiance[1] += elem->sourceRad.getG();
        radiance[2] += elem->sourceRad.getB();
        if ( StochasticRelaxation::activeState().indirectOnly || StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
            // Add self-emitted radiance
            radiance[0] += elem->Ed.getR();
            radiance[1] += elem->Ed.getG();
            radiance[2] += elem->Ed.getB();
        }
    }
    return ColorRgb(radiance[0], radiance[1], radiance[2]);
}

ColorRgb
StochasticRadiosityElement::stochasticRadiosityElementDisplayRadianceAtPoint(const StochasticRadiosityElement *elem, double u, double v, const RendererConfiguration *renderOptions) {
    ColorRgb radiance(0.0, 0.0, 0.0);
    if ( elem->basis->size == 1 ) {
        if ( renderOptions->isSmoothShading() ) {
            // Do Gouraud interpolation if required
            ColorRgb rad[4];
            for ( int i = 0; i < elem->numberOfVertices; i++ ) {
                rad[i] = vertexRadiance(elem->vertices[i]);
            }
            const double x = static_cast<double>(u);
            const double y = static_cast<double>(v);
            switch ( elem->numberOfVertices ) {
                case 3:
                    radiance = ColorRgb(
                        rad[0].getR() + x * (rad[1].getR() - rad[0].getR()) + y * (rad[2].getR() - rad[0].getR()),
                        rad[0].getG() + x * (rad[1].getG() - rad[0].getG()) + y * (rad[2].getG() - rad[0].getG()),
                        rad[0].getB() + x * (rad[1].getB() - rad[0].getB()) + y * (rad[2].getB() - rad[0].getB()));
                    break;
                case 4: {
                    const double c = x * y;
                    const double bb = x - c;
                    const double d = y - c;
                    radiance = ColorRgb(
                        rad[0].getR() + bb * (rad[1].getR() - rad[0].getR()) + c * (rad[2].getR() - rad[0].getR()) + d * (rad[3].getR() - rad[0].getR()),
                        rad[0].getG() + bb * (rad[1].getG() - rad[0].getG()) + c * (rad[2].getG() - rad[0].getG()) + d * (rad[3].getG() - rad[0].getG()),
                        rad[0].getB() + bb * (rad[1].getB() - rad[0].getB()) + c * (rad[2].getB() - rad[0].getB()) + d * (rad[3].getB() - rad[0].getB()));
                    break;
                }
                default:
                    Logger::reportMessage("StochasticRadiosityElementRender", Logger::FATAL_ERROR, "stochasticRadiosityElementDisplayRadianceAtPoint", "can only handle triangular or quadrilateral elements");
            }
        } else {
            // Flat shading
            radiance = StochasticRadiosityElement::stochasticRadiosityElementDisplayRadiance(elem);
        }
    } else {
        // Higher order approximations
        radiance = Basismcrad::colorAtUv(elem->basis, elem->radiance, u, v);
        if ( StochasticRelaxation::activeState().show == WhatToShow::SHOW_INDIRECT_RADIANCE ) {
            radiance = ColorRgb(radiance.getR() - elem->sourceRad.getR(),
                                radiance.getG() - elem->sourceRad.getG(),
                                radiance.getB() - elem->sourceRad.getB());
        }
    }
    return radiance;
}

#endif
