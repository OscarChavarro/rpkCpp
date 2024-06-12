/**
Rendering elements
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "tonemap/ToneMap.h"
#include "render/opengl.h"
#include "render/render.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

ColorRgb
stochasticRadiosityElementColor(const StochasticRadiosityElement *element) {
    ColorRgb color{};

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show ) {
        case WhatToShow::SHOW_TOTAL_RADIANCE:
        case WhatToShow::SHOW_INDIRECT_RADIANCE:
            radianceToRgb(stochasticRadiosityElementDisplayRadiance(element), &color);
            break;
        case WhatToShow::SHOW_IMPORTANCE: {
            float gray;

            if ( element->importance > 1.0 ) {
                gray = 1.0f;
            } else {
                gray = element->importance < 0.0 ? 0.0f : element->importance;
            }

            color.set(gray, gray, gray);
            break;
        }
        default:
            logFatal(
                -1,
                "stochasticRadiosityElementColor",
                "Don't know what to display (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show = %d)",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show);
    }

    return color;
}

static ColorRgb
vertexRadiance(const Vertex *v) {
    int count = 0;
    ColorRgb radiance;

    radiance.clear();
    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *element = v->radianceData->get(i);
        if ( element->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *elem = (StochasticRadiosityElement *)element;
        if ( !elem->regularSubElements ) {
            ColorRgb elementRadiosity = stochasticRadiosityElementDisplayRadiance(elem);
            radiance.add(radiance, elementRadiosity);
            count++;
        }
    }

    if ( count > 0 ) {
        radiance.scaleInverse((float) count, radiance);
    }

    return radiance;
}

/**
Same as above but for importance
*/
static float
vertexImportance(const Vertex *v) {
    int count = 0;
    float imp = 0.0;

    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *element = (StochasticRadiosityElement *)genericElement;
        if ( !element->regularSubElements ) {
            imp += element->importance;
            count++;
        }
    }

    if ( count > 0 ) {
        imp /= (float) count;
    }

    return imp;
}

ColorRgb
vertexColor(Vertex *v) {
    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show ) {
        case WhatToShow::SHOW_TOTAL_RADIANCE:
        case WhatToShow::SHOW_INDIRECT_RADIANCE:
            radianceToRgb(vertexRadiance(v), &v->color);
            break;
        case WhatToShow::SHOW_IMPORTANCE: {
            float gray = vertexImportance(v);
            if ( gray > 1.0 ) {
                gray = 1.0;
            }
            if ( gray < 0.0 ) {
                gray = 0.0;
            }
            v->color.set(gray, gray, gray);
            break;
        }
        default:
            logFatal(-1, "vertexColor",
                     "Don't know what to display (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show = %d)",
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show);
    }

    return v->color;
}

/**
Compute new vertex colors
*/
void
stochasticRadiosityElementComputeNewVertexColors(Element *element) {
    const StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    vertexColor(stochasticRadiosityElement->vertices[0]);
    vertexColor(stochasticRadiosityElement->vertices[1]);
    vertexColor(stochasticRadiosityElement->vertices[2]);
    if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
        vertexColor(stochasticRadiosityElement->vertices[3]);
    }
}

void
stochasticRadiosityElementAdjustTVertexColors(Element *element) {
    const StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    Vertex *m[4];
    int i;
    int n;
    for ( i = 0, n = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
        m[i] = stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( n > 0 ) {
        ColorRgb color = stochasticRadiosityElementColor(stochasticRadiosityElement);
        for ( i = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
            if ( m[i] ) {
                m[i]->color.r = (m[i]->color.r + color.r) * 0.5f;
                m[i]->color.g = (m[i]->color.g + color.g) * 0.5f;
                m[i]->color.b = (m[i]->color.b + color.b) * 0.5f;
            }
        }
    }
}

static void
renderTriangle(const Vertex *v1, const Vertex *v2, const Vertex *v3, const RenderOptions *renderOptions) {
    ColorRgb col[3];
    Vector3D vert[3];

    vert[0] = *(v1->point);
    col[0] = v1->color;
    vert[1] = *(v2->point);
    col[1] = v2->color;
    vert[2] = *(v3->point);
    col[2] = v3->color;
    openGlRenderPolygonGouraud(3, vert, col);

    if ( renderOptions->drawOutlines ) {
        openGlRenderSetColor(&renderOptions->outlineColor);
        openGlRenderLine(&vert[0], &vert[1]);
        openGlRenderLine(&vert[1], &vert[2]);
        openGlRenderLine(&vert[2], &vert[0]);
    }
}

static void
renderQuadrilateral(const Vertex *v1, const Vertex *v2, const Vertex *v3, const Vertex *v4, const RenderOptions *renderOptions) {
    ColorRgb col[4];
    Vector3D vert[4];

    vert[0] = *(v1->point);
    col[0] = v1->color;
    vert[1] = *(v2->point);
    col[1] = v2->color;
    vert[2] = *(v3->point);
    col[2] = v3->color;
    vert[3] = *(v4->point);
    col[3] = v4->color;
    openGlRenderPolygonGouraud(4, vert, col);

    if ( renderOptions->drawOutlines ) {
        openGlRenderSetColor(&renderOptions->outlineColor);
        openGlRenderLine(&vert[0], &vert[1]);
        openGlRenderLine(&vert[1], &vert[2]);
        openGlRenderLine(&vert[2], &vert[3]);
        openGlRenderLine(&vert[3], &vert[0]);
    }
}

/**
TODO: The T-vertex elimination code only fully eliminates T-vertices in
balanced meshes. Meshes are not yet balanced [PhB - 9901]
*/
static void
triangleTVertexElimination(
    Vertex **v,
    Vertex **m,
    int numberOfTVertices,
    void (*doTriangleCallback)(const Vertex *, const Vertex *, const Vertex *, const RenderOptions *),
    const RenderOptions *renderOptions)
{
    int a;
    int b;
    int c;

    switch ( numberOfTVertices ) {
        case 0:
            doTriangleCallback(v[0], v[1], v[2], renderOptions);
            break;
        case 1:
            for ( a = 0; a < 3; a++ ) {
                if ( m[a] ) {
                    break;
                }
            }
            b = (a + 1) % 3;
            c = (a + 2) % 3;
            doTriangleCallback(m[a], v[c], v[a], renderOptions);
            doTriangleCallback(m[a], v[b], v[c], renderOptions);
            break;
        case 2:
            for ( a = 0; a < 3; a++ ) {
                if ( !m[a] ) {
                    break;
                }
            }
            b = (a + 1) % 3;
            c = (a + 2) % 3;
            doTriangleCallback(m[b], v[a], v[b], renderOptions);
            doTriangleCallback(m[b], m[c], v[a], renderOptions);
            doTriangleCallback(m[b], v[c], m[c], renderOptions);
            break;
        case 3:
            doTriangleCallback(v[0], m[0], m[2], renderOptions);
            doTriangleCallback(v[1], m[1], m[0], renderOptions);
            doTriangleCallback(v[2], m[2], m[1], renderOptions);
            doTriangleCallback(m[0], m[1], m[2], renderOptions);
            break;
        default:
            break;
    }
}

static void
quadrilateralTVertexElimination(
    Vertex **v,
    Vertex **m,
    int numberOfTVertices,
    void (*doTriangleCallback)(const Vertex *, const Vertex *, const Vertex *, const RenderOptions *),
    void (*doQuadrilateralCallback)(const Vertex *, const Vertex *, const Vertex *, const Vertex *, const RenderOptions *),
    const RenderOptions *renderOptions)
{
    int a;
    int b;
    int c;
    int d;

    switch ( numberOfTVertices ) {
        case 0:
            doQuadrilateralCallback(v[0], v[1], v[2], v[3], renderOptions);
            break;
        case 1:
            for ( a = 0; a < 4; a++ ) {
                if ( m[a] ) {
                    break;
                }
            }
            b = (a + 1) % 4;
            c = (a + 2) % 4;
            d = (a + 3) % 4;
            doTriangleCallback(m[a], v[d], v[a], renderOptions);
            doTriangleCallback(m[a], v[c], v[d], renderOptions);
            doTriangleCallback(m[a], v[b], v[c], renderOptions);
            break;
        case 2:
            for ( a = 0; a < 4; a++ ) {
                if ( m[a] ) {
                    break;
                }
            }
            b = (a + 1) % 4;
            c = (a + 2) % 4;
            d = (a + 3) % 4;
            if ( m[d] ) {
                a = d;
                b = (a + 1) % 4;
                c = (a + 2) % 4;
                d = (a + 3) % 4;
            }
            if ( m[b] ) {
                doTriangleCallback(m[a], v[b], m[b], renderOptions);
                doTriangleCallback(m[b], v[c], v[d], renderOptions);
                doTriangleCallback(v[d], m[a], m[b], renderOptions);
                doTriangleCallback(v[d], v[a], m[a], renderOptions);
            } else {
                doQuadrilateralCallback(v[a], m[a], m[c], v[d], renderOptions);
                doQuadrilateralCallback(m[a], v[b], v[c], m[c], renderOptions);
            }
            break;
        case 3:
            for ( a = 0; a < 4; a++ ) {
                if ( !m[a] ) {
                    break;
                }
            }
            b = (a + 1) % 4;
            c = (a + 2) % 4;
            d = (a + 3) % 4;
            doQuadrilateralCallback(v[a], v[b], m[b], m[d], renderOptions);
            doTriangleCallback(m[b], v[c], m[c], renderOptions);
            doTriangleCallback(m[c], v[d], m[d], renderOptions);
            doTriangleCallback(m[b], m[c], m[d], renderOptions);
            break;
        case 4:
            doTriangleCallback(v[0], m[0], m[3], renderOptions);
            doTriangleCallback(v[1], m[1], m[0], renderOptions);
            doTriangleCallback(v[2], m[2], m[1], renderOptions);
            doTriangleCallback(v[3], m[3], m[2], renderOptions);
            doQuadrilateralCallback(m[0], m[1], m[2], m[3], renderOptions);
            break;
        default:
            break;
    }
}

static void
renderTriangularElement(Vertex **v, Vertex **m, int numberOfTVertices, const RenderOptions *renderOptions) {
    triangleTVertexElimination(v, m, numberOfTVertices, renderTriangle, renderOptions);
}

static void
renderQuadrilateralElement(Vertex **v, Vertex **m, int numberOfTVertices, const RenderOptions *renderOptions) {
    quadrilateralTVertexElimination(v, m, numberOfTVertices, renderTriangle, renderQuadrilateral, renderOptions);
}

static void
stochasticRadiosityElementRenderOutline(const StochasticRadiosityElement *elem, const RenderOptions *renderOptions) {
    Vector3D vertices[4];

    openGlRenderSetColor(&renderOptions->outlineColor);
    openGlRenderLine(&vertices[0], &vertices[1]);
    openGlRenderLine(&vertices[1], &vertices[2]);
    if ( elem->numberOfVertices == 3 ) {
        openGlRenderLine(&vertices[2], &vertices[0]);
    } else {
        openGlRenderLine(&vertices[2], &vertices[3]);
        openGlRenderLine(&vertices[3], &vertices[0]);
    }
}

void
stochasticRadiosityElementRender(Element *element, const RenderOptions *renderOptions) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    Vector3D vertices[4];

    if ( renderOptions->smoothShading && GLOBAL_stochasticRaytracing_hierarchy.tvertex_elimination ) {
        Vertex *m[4]{};
        int n = 0;
        for ( int i = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
            m[i] = stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
            if ( m[i] ) {
                n++;
            }
        }

        if ( stochasticRadiosityElement->numberOfVertices == 3 ) {
            renderTriangularElement(stochasticRadiosityElement->vertices, m, n, renderOptions);
        } else {
            renderQuadrilateralElement(stochasticRadiosityElement->vertices, m, n, renderOptions);
        }
        return;
    }

    vertices[0] = *(stochasticRadiosityElement->vertices[0]->point);
    vertices[1] = *(stochasticRadiosityElement->vertices[1]->point);
    vertices[2] = *(stochasticRadiosityElement->vertices[2]->point);
    if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
        vertices[3] = *(stochasticRadiosityElement->vertices[3]->point);
    }

    if ( renderOptions->smoothShading ) {
        ColorRgb vertexColors[4];
        vertexColors[0] = stochasticRadiosityElement->vertices[0]->color;
        vertexColors[1] = stochasticRadiosityElement->vertices[1]->color;
        vertexColors[2] = stochasticRadiosityElement->vertices[2]->color;
        if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
            vertexColors[3] = stochasticRadiosityElement->vertices[3]->color;
        }

        openGlRenderPolygonGouraud(stochasticRadiosityElement->numberOfVertices, vertices, vertexColors);
    } else {
        ColorRgb color = stochasticRadiosityElementColor(stochasticRadiosityElement);

        openGlRenderSetColor(&color);
        openGlRenderPolygonFlat(stochasticRadiosityElement->numberOfVertices, vertices);
    }

    if ( renderOptions->drawOutlines )
        stochasticRadiosityElementRenderOutline(stochasticRadiosityElement, renderOptions);
}

ColorRgb
stochasticRadiosityElementDisplayRadiance(const StochasticRadiosityElement *elem) {
    ColorRgb radiance;
    radiance.subtract(elem->radiance[0], elem->sourceRad);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show != WhatToShow::SHOW_INDIRECT_RADIANCE ) {
        // Source_rad is self-emitted radiance if !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly. It is direct
        // illumination if GLOBAL_stochasticRaytracing_monteCarloRadiosityState.direct_only */
        radiance.add(radiance, elem->sourceRad);
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly || GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            // Add self-emitted radiance
            radiance.add(radiance, elem->Ed);
        }
    }
    return radiance;
}

ColorRgb
stochasticRadiosityElementDisplayRadianceAtPoint(const StochasticRadiosityElement *elem, double u, double v, const RenderOptions *renderOptions) {
    ColorRgb radiance;
    if ( elem->basis->size == 1 ) {
        if ( renderOptions->smoothShading ) {
            // Do Gouraud interpolation if required
            ColorRgb rad[4];
            for ( int i = 0; i < elem->numberOfVertices; i++ ) {
                rad[i] = vertexRadiance(elem->vertices[i]);
            }
            switch ( elem->numberOfVertices ) {
                case 3:
                    radiance.interpolateBarycentric(rad[0], rad[1], rad[2], (float) u, (float) v);
                    break;
                case 4:
                    radiance.interpolateBiLinear(rad[0], rad[1], rad[2], rad[3], (float) u, (float) v);
                    break;
                default:
                    logFatal(-1, "stochasticRadiosityElementDisplayRadianceAtPoint",
                             "can only handle triangular or quadrilateral elements");
            }
        } else {
            // Flat shading
            radiance = stochasticRadiosityElementDisplayRadiance(elem);
        }
    } else {
        // Higher order approximations
        radiance = colorAtUv(elem->basis, elem->radiance, u, v);
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show == WhatToShow::SHOW_INDIRECT_RADIANCE ) {
            radiance.subtract(radiance, elem->sourceRad);
        }
    }
    return radiance;
}

#endif
