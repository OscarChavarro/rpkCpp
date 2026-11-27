#include "vsdk/toolkit/material/RendererConfiguration.h"

/**
Rendering elements
*/


#ifdef RAYTRACING_ENABLED
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

ColorRgb
StochasticRadiosityElement::stochasticRadiosityElementColor(const StochasticRadiosityElement *element) {
    ColorRgb color = ColorRgb();

    switch ( StochasticRelaxation::activeState().show ) {
        case SHOW_TOTAL_RADIANCE:
        case SHOW_INDIRECT_RADIANCE:
            ToneMap::radianceToRgb(
                StochasticRadiosityElement::stchsRadElemDispRadn(element),
                &color,
                *StochasticRelaxation::activeState().toneMapOptions);
            break;
        case SHOW_IMPORTANCE: {
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
            Logger::reportMessage("StochasticRadiosityElementRender", Logger::FATAL_ERROR, "stochasticRadiosityElementColor", LoggerFormat::format("Don't know what to display (StochasticRelaxation::activeState().show = %d)", StochasticRelaxation::activeState().show));
    }

    return color;
}

ColorRgb
StochasticRadiosityElement::vertexRadiance(const RpkVertex *v) {
    int count = 0;
    ColorRgb radiance;

    radiance = ColorRgb(0.0, 0.0, 0.0);
    for ( int i = 0; v->radianceData != NULL && i < v->radianceData->size(); i++ ) {
        Element *element = v->radianceData->get(i);
        if ( element->className != ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *elem = ((StochasticRadiosityElement *)(element));
        if ( !elem->regularSubElements ) {
            ColorRgb elementRadiosity = StochasticRadiosityElement::stchsRadElemDispRadn(elem);
            radiance = ColorRgb(
                radiance.getR() + elementRadiosity.getR(),
                radiance.getG() + elementRadiosity.getG(),
                radiance.getB() + elementRadiosity.getB());
            count++;
        }
    }

    if ( count > 0 ) {
        double inv = 1.0 / ((double)(count));
        radiance = ColorRgb(inv * radiance.getR(), inv * radiance.getG(), inv * radiance.getB());
    }

    return radiance;
}

/**
Same as above but for importance
*/
double
StochasticRadiosityElement::vertexImportance(const RpkVertex *v) {
    int count = 0;
    double imp = 0.0;

    for ( int i = 0; v->radianceData != NULL && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *element = ((StochasticRadiosityElement *)(genericElement));
        if ( !element->regularSubElements ) {
            imp += element->importance;
            count++;
        }
    }

    if ( count > 0 ) {
        imp /= ((double)(count));
    }

    return imp;
}

ColorRgb
StochasticRadiosityElement::vertexColor(RpkVertex *v) {
    switch ( StochasticRelaxation::activeState().show ) {
        case SHOW_TOTAL_RADIANCE:
        case SHOW_INDIRECT_RADIANCE:
            ToneMap::radianceToRgb(
                vertexRadiance(v),
                &v->color,
                *StochasticRelaxation::activeState().toneMapOptions);
            break;
        case SHOW_IMPORTANCE: {
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
            Logger::reportMessage("StochasticRadiosityElementRender", Logger::FATAL_ERROR, "vertexColor", LoggerFormat::format("Don't know what to display (StochasticRelaxation::activeState().show = %d)", StochasticRelaxation::activeState().show));
    }

    return v->color;
}

/**
Compute new vertex colors
*/
void
StochasticRadiosityElement::stchsRadElemCompNewVtxClrs(Element *element) {
    const StochasticRadiosityElement *stochasticRadiosityElement = ((StochasticRadiosityElement *)(element));
    vertexColor(stochasticRadiosityElement->vertices[0]);
    vertexColor(stochasticRadiosityElement->vertices[1]);
    vertexColor(stochasticRadiosityElement->vertices[2]);
    if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
        vertexColor(stochasticRadiosityElement->vertices[3]);
    }
}

void
StochasticRadiosityElement::stchsRadElemAdjTVtxClrs(Element *element) {
    const StochasticRadiosityElement *stochasticRadiosityElement = ((StochasticRadiosityElement *)(element));
    RpkVertex *m[4];
    int i;
    int n;
    for ( i = 0, n = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
        m[i] = StochasticRadiosityElement::stchsRadElemEdgeMidVtx(stochasticRadiosityElement, i);
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
StochasticRadiosityElement::stchsRadElemDispRadn(const StochasticRadiosityElement *elem) {
    ColorRgb radiance(
        elem->radiance[0].getR() - elem->sourceRad.getR(),
        elem->radiance[0].getG() - elem->sourceRad.getG(),
        elem->radiance[0].getB() - elem->sourceRad.getB());

    if ( StochasticRelaxation::activeState().show != SHOW_INDIRECT_RADIANCE ) {
        // sourceRad is self-emitted radiance when indirect-only is disabled.
        // Otherwise it represents direct illumination.
        radiance = ColorRgb(
            radiance.getR() + elem->sourceRad.getR(),
            radiance.getG() + elem->sourceRad.getG(),
            radiance.getB() + elem->sourceRad.getB());
        if ( StochasticRelaxation::activeState().indirectOnly || StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
            // Add self-emitted radiance
            radiance = ColorRgb(
                radiance.getR() + elem->Ed.getR(),
                radiance.getG() + elem->Ed.getG(),
                radiance.getB() + elem->Ed.getB());
        }
    }
    return radiance;
}

ColorRgb
StochasticRadiosityElement::stchsRadElemDispRadnAPnt(const StochasticRadiosityElement *elem, double u, double v, const RenderOptions *renderOptions) {
    ColorRgb radiance;
    if ( elem->basis->size == 1 ) {
        if ( renderOptions->isSmoothShading() ) {
            // Do Gouraud interpolation if required
            ColorRgb rad[4];
            for ( int i = 0; i < elem->numberOfVertices; i++ ) {
                rad[i] = vertexRadiance(elem->vertices[i]);
            }
            switch ( elem->numberOfVertices ) {
                case 3:
                    radiance = ColorRgb(
                        (1.0 - ((double)(u)) - ((double)(v))) * rad[0].getR() + ((double)(u)) * rad[1].getR() + ((double)(v)) * rad[2].getR(),
                        (1.0 - ((double)(u)) - ((double)(v))) * rad[0].getG() + ((double)(u)) * rad[1].getG() + ((double)(v)) * rad[2].getG(),
                        (1.0 - ((double)(u)) - ((double)(v))) * rad[0].getB() + ((double)(u)) * rad[1].getB() + ((double)(v)) * rad[2].getB());
                    break;
                case 4:
                    radiance = ColorRgb(
                        (1.0 - ((double)(u))) * (1.0 - ((double)(v))) * rad[0].getR() + ((double)(u)) * (1.0 - ((double)(v))) * rad[1].getR() + ((double)(u)) * ((double)(v)) * rad[2].getR() + (1.0 - ((double)(u))) * ((double)(v)) * rad[3].getR(),
                        (1.0 - ((double)(u))) * (1.0 - ((double)(v))) * rad[0].getG() + ((double)(u)) * (1.0 - ((double)(v))) * rad[1].getG() + ((double)(u)) * ((double)(v)) * rad[2].getG() + (1.0 - ((double)(u))) * ((double)(v)) * rad[3].getG(),
                        (1.0 - ((double)(u))) * (1.0 - ((double)(v))) * rad[0].getB() + ((double)(u)) * (1.0 - ((double)(v))) * rad[1].getB() + ((double)(u)) * ((double)(v)) * rad[2].getB() + (1.0 - ((double)(u))) * ((double)(v)) * rad[3].getB());
                    break;
                default:
                    Logger::reportMessage("StochasticRadiosityElementRender", Logger::FATAL_ERROR, "stchsRadElemDispRadnAPnt", "can only handle triangular or quadrilateral elements");
            }
        } else {
            // Flat shading
            radiance = StochasticRadiosityElement::stchsRadElemDispRadn(elem);
        }
    } else {
        // Higher order approximations
        radiance = Basismcrad::colorAtUv(elem->basis, elem->radiance, u, v);
        if ( StochasticRelaxation::activeState().show == SHOW_INDIRECT_RADIANCE ) {
            radiance = ColorRgb(
                radiance.getR() - elem->sourceRad.getR(),
                radiance.getG() - elem->sourceRad.getG(),
                radiance.getB() - elem->sourceRad.getB());
        }
    }
    return radiance;
}

#endif
