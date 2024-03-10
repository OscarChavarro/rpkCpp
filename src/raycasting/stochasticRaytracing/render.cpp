/**
Rendering elements
*/

#include "common/error.h"
#include "render/render.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "render/opengl.h"

RGB
stochasticRadiosityElementColor(StochasticRadiosityElement *element) {
    RGB color{};

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show ) {
        case SHOW_TOTAL_RADIANCE:
        case SHOW_INDIRECT_RADIANCE:
            radianceToRgb(stochasticRadiosityElementDisplayRadiance(element), &color);
            break;
        case SHOW_IMPORTANCE: {
            float gray = element->importance > 1.0 ? 1.0f : element->importance < 0.0 ? 0.0f : element->importance;
            setRGB(color, gray, gray, gray);
            break;
        }
        default:
            logFatal(-1, "stochasticRadiosityElementColor",
                     "Don't know what to display (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show = %d)",
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show);
    }

    return color;
}

static COLOR
vertexRadiance(Vertex *v) {
    int count = 0;
    COLOR radiance;

    colorClear(radiance);
    for ( int i = 0; v->radiance_data != nullptr && i < v->radiance_data->size(); i++ ) {
        Element *element = v->radiance_data->get(i);
        if ( element->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        StochasticRadiosityElement *elem = (StochasticRadiosityElement *)element;
        if ( !elem->regularSubElements ) {
            COLOR elementRadiosity = stochasticRadiosityElementDisplayRadiance(elem);
            colorAdd(radiance, elementRadiosity, radiance);
            count++;
        }
    }

    if ( count > 0 ) {
        colorScaleInverse((float) count, radiance, radiance);
    }

    return radiance;
}

/**
Same as above but for importance
*/
static float
vertexImportance(Vertex *v) {
    int count = 0;
    float imp = 0.0;

    for ( int i = 0; v->radiance_data != nullptr && i < v->radiance_data->size(); i++ ) {
        Element *genericElement = v->radiance_data->get(i);
        if ( genericElement->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        StochasticRadiosityElement *element = (StochasticRadiosityElement *)genericElement;
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

RGB
vertexColor(Vertex *v) {
    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show ) {
        case SHOW_TOTAL_RADIANCE:
        case SHOW_INDIRECT_RADIANCE:
            radianceToRgb(vertexRadiance(v), &v->color);
            break;
        case SHOW_IMPORTANCE: {
            float gray = vertexImportance(v);
            if ( gray > 1.0 ) {
                gray = 1.0;
            }
            if ( gray < 0.0 ) {
                gray = 0.0;
            }
            setRGB(v->color, gray, gray, gray);
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
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    vertexColor(stochasticRadiosityElement->vertices[0]);
    vertexColor(stochasticRadiosityElement->vertices[1]);
    vertexColor(stochasticRadiosityElement->vertices[2]);
    if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
        vertexColor(stochasticRadiosityElement->vertices[3]);
    }
}

void
stochasticRadiosityElementAdjustTVertexColors(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    Vertex *m[4];
    int i, n;
    for ( i = 0, n = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
        m[i] = stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( n > 0 ) {
        RGB color = stochasticRadiosityElementColor(stochasticRadiosityElement);
        for ( i = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
            if ( m[i] ) {
                m[i]->color.r = (float)(m[i]->color.r + color.r) * 0.5f;
                m[i]->color.g = (float)(m[i]->color.g + color.g) * 0.5f;
                m[i]->color.b = (float)(m[i]->color.b + color.b) * 0.5f;
            }
        }
    }
}

static void
renderTriangle(Vertex *v1, Vertex *v2, Vertex *v3) {
    RGB col[3];
    Vector3D vert[3];

    vert[0] = *(v1->point);
    col[0] = v1->color;
    vert[1] = *(v2->point);
    col[1] = v2->color;
    vert[2] = *(v3->point);
    col[2] = v3->color;
    openGlRenderPolygonGouraud(3, vert, col);

    if ( GLOBAL_render_renderOptions.drawOutlines ) {
        int i;
        for ( i = 0; i < 3; i++ ) {
            Vector3D d;
            vectorSubtract(GLOBAL_camera_mainCamera.eyePosition, vert[i], d);
            vectorSumScaled(vert[i], 0.01, d, vert[i]);
        }

        openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
        openGlRenderLine(&vert[0], &vert[1]);
        openGlRenderLine(&vert[1], &vert[2]);
        openGlRenderLine(&vert[2], &vert[0]);
    }
}

static void
renderQuadrilateral(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4) {
    RGB col[4];
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

    if ( GLOBAL_render_renderOptions.drawOutlines ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            Vector3D d;
            vectorSubtract(GLOBAL_camera_mainCamera.eyePosition, vert[i], d);
            vectorSumScaled(vert[i], 0.01, d, vert[i]);
        }

        openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
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
    void (*do_triangle)(Vertex *, Vertex *, Vertex *))
{
    int a;
    int b;
    int c;

    switch ( numberOfTVertices ) {
        case 0:
            do_triangle(v[0], v[1], v[2]);
            break;
        case 1:
            for ( a = 0; a < 3; a++ ) {
                if ( m[a] ) {
                    break;
                }
            }
            b = (a + 1) % 3;
            c = (a + 2) % 3;
            do_triangle(m[a], v[c], v[a]);
            do_triangle(m[a], v[b], v[c]);
            break;
        case 2:
            for ( a = 0; a < 3; a++ ) {
                if ( !m[a] ) {
                    break;
                }
            }
            b = (a + 1) % 3;
            c = (a + 2) % 3;
            do_triangle(m[b], v[a], v[b]);
            do_triangle(m[b], m[c], v[a]);
            do_triangle(m[b], v[c], m[c]);
            break;
        case 3:
            do_triangle(v[0], m[0], m[2]);
            do_triangle(v[1], m[1], m[0]);
            do_triangle(v[2], m[2], m[1]);
            do_triangle(m[0], m[1], m[2]);
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
    void (*do_triangle)(Vertex *, Vertex *, Vertex *),
    void (*do_quadrilateral)(Vertex *, Vertex *, Vertex *, Vertex *))
{
    int a;
    int b;
    int c;
    int d;

    switch ( numberOfTVertices ) {
        case 0:
            do_quadrilateral(v[0], v[1], v[2], v[3]);
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
            do_triangle(m[a], v[d], v[a]);
            do_triangle(m[a], v[c], v[d]);
            do_triangle(m[a], v[b], v[c]);
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
                do_triangle(m[a], v[b], m[b]);
                do_triangle(m[b], v[c], v[d]);
                do_triangle(v[d], m[a], m[b]);
                do_triangle(v[d], v[a], m[a]);
            } else {
                do_quadrilateral(v[a], m[a], m[c], v[d]);
                do_quadrilateral(m[a], v[b], v[c], m[c]);
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
            do_quadrilateral(v[a], v[b], m[b], m[d]);
            do_triangle(m[b], v[c], m[c]);
            do_triangle(m[c], v[d], m[d]);
            do_triangle(m[b], m[c], m[d]);
            break;
        case 4:
            do_triangle(v[0], m[0], m[3]);
            do_triangle(v[1], m[1], m[0]);
            do_triangle(v[2], m[2], m[1]);
            do_triangle(v[3], m[3], m[2]);
            do_quadrilateral(m[0], m[1], m[2], m[3]);
            break;
        default:
            break;
    }
}

static void
renderTriangularElement(Vertex **v, Vertex **m, int numberOfTVertices) {
    triangleTVertexElimination(v, m, numberOfTVertices, renderTriangle);
}

static void
renderQuadrilateralElement(Vertex **v, Vertex **m, int numberOfTVertices) {
    quadrilateralTVertexElimination(v, m, numberOfTVertices, renderTriangle, renderQuadrilateral);
}

void
stochasticRadiosityElementTVertexElimination(
    StochasticRadiosityElement *elem,
    void (*do_triangle)(Vertex *, Vertex *, Vertex *),
    void (*do_quadrilateral)(Vertex *, Vertex *, Vertex *, Vertex *))
{
    Vertex *m[4];
    int i, n;
    for ( i = 0, n = 0; i < elem->numberOfVertices; i++ ) {
        m[i] = stochasticRadiosityElementEdgeMidpointVertex(elem, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( elem->numberOfVertices == 3 ) {
        triangleTVertexElimination(elem->vertices, m, n, do_triangle);
    } else {
        quadrilateralTVertexElimination(elem->vertices, m, n, do_triangle, do_quadrilateral);
    }
}

void
stochasticRadiosityElementRenderOutline(StochasticRadiosityElement *elem) {
    Vector3D vertices[4];

    // Test whether eye point is in front of the patch
    if ( vectorDotProduct(elem->patch->normal, GLOBAL_camera_mainCamera.eyePosition) + elem->patch->planeConstant < 0.0 ) {
        return;
    }

    for ( int i = 0; i < elem->numberOfVertices; i++ ) {
        Vector3D d;
        vertices[i] = *(elem->vertices[i]->point);
        vectorSubtract(GLOBAL_camera_mainCamera.eyePosition, vertices[i], d);
        vectorSumScaled(vertices[i], 0.0001, d, vertices[i]);
    }

    openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
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
stochasticRadiosityElementRender(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    Vector3D vertices[4];

    if ( GLOBAL_render_renderOptions.smoothShading && GLOBAL_stochasticRaytracing_hierarchy.tvertex_elimination ) {
        Vertex *m[4];
        int i, n;
        for ( i = 0, n = 0; i < stochasticRadiosityElement->numberOfVertices; i++ ) {
            m[i] = stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
            if ( m[i] ) {
                n++;
            }
        }

        if ( stochasticRadiosityElement->numberOfVertices == 3 ) {
            renderTriangularElement(stochasticRadiosityElement->vertices, m, n);
        } else {
            renderQuadrilateralElement(stochasticRadiosityElement->vertices, m, n);
        }
        return;
    }

    vertices[0] = *(stochasticRadiosityElement->vertices[0]->point);
    vertices[1] = *(stochasticRadiosityElement->vertices[1]->point);
    vertices[2] = *(stochasticRadiosityElement->vertices[2]->point);
    if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
        vertices[3] = *(stochasticRadiosityElement->vertices[3]->point);
    }

    if ( GLOBAL_render_renderOptions.smoothShading ) {
        RGB vertexColors[4];
        vertexColors[0] = stochasticRadiosityElement->vertices[0]->color;
        vertexColors[1] = stochasticRadiosityElement->vertices[1]->color;
        vertexColors[2] = stochasticRadiosityElement->vertices[2]->color;
        if ( stochasticRadiosityElement->numberOfVertices > 3 ) {
            vertexColors[3] = stochasticRadiosityElement->vertices[3]->color;
        }

        openGlRenderPolygonGouraud(stochasticRadiosityElement->numberOfVertices, vertices, vertexColors);
    } else {
        RGB color = stochasticRadiosityElementColor(stochasticRadiosityElement);

        openGlRenderSetColor(&color);
        openGlRenderPolygonFlat(stochasticRadiosityElement->numberOfVertices, vertices);
    }

    if ( GLOBAL_render_renderOptions.drawOutlines )
        stochasticRadiosityElementRenderOutline(stochasticRadiosityElement);
}

COLOR
stochasticRadiosityElementDisplayRadiance(StochasticRadiosityElement *elem) {
    COLOR rad;
    colorSubtract(elem->radiance[0], elem->sourceRad, rad);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show != SHOW_INDIRECT_RADIANCE ) {
        // Source_rad is self-emitted radiance if !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly. It is direct
        // illumination if GLOBAL_stochasticRaytracing_monteCarloRadiosityState.direct_only */
        colorAdd(rad, elem->sourceRad, rad);
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly || GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            // Add self-emitted radiance
            colorAdd(rad, elem->Ed, rad);
        }
    }
    return rad;
}

COLOR
stochasticRadiosityElementDisplayRadianceAtPoint(StochasticRadiosityElement *elem, double u, double v) {
    COLOR radiance;
    if ( elem->basis->size == 1 ) {
        if ( GLOBAL_render_renderOptions.smoothShading ) {
            // Do Gouraud interpolation if required
            int i;
            COLOR rad[4];
            for ( i = 0; i < elem->numberOfVertices; i++ ) {
                rad[i] = vertexRadiance(elem->vertices[i]);
            }
            switch ( elem->numberOfVertices ) {
                case 3:
                    colorInterpolateBarycentric(rad[0], rad[1], rad[2], (float)u, (float)v, radiance);
                    break;
                case 4:
                    colorInterpolateBiLinear(rad[0], rad[1], rad[2], rad[3], (float) u, (float) v, radiance);
                    break;
                default:
                    logFatal(-1, "stochasticRadiosityElementDisplayRadianceAtPoint",
                             "can only handle triangular or quadrilateral elements");
                    colorClear(radiance);
            }
        } else {
            // Flat shading
            radiance = stochasticRadiosityElementDisplayRadiance(elem);
        }
    } else {
        // Higher order approximations
        radiance = colorAtUv(elem->basis, elem->radiance, u, v);
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show == SHOW_INDIRECT_RADIANCE ) {
            colorSubtract(radiance, elem->sourceRad, radiance);
        }
    }
    return radiance;
}
