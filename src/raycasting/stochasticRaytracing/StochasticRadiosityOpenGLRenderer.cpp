#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "render/Opengl.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityOpenGLRenderer.h"

void
StochasticRadiosityOpenGLRenderer::renderTriangle(
    const Vertex *v1,
    const Vertex *v2,
    const Vertex *v3,
    const RenderOptions *renderOptions)
{
    ColorRgb colors[3];
    Vector3D vertices[3];

    vertices[0] = *(v1->point);
    colors[0] = v1->color;
    vertices[1] = *(v2->point);
    colors[1] = v2->color;
    vertices[2] = *(v3->point);
    colors[2] = v3->color;
    Opengl::openGlRenderPolygonGouraud(3, vertices, colors, renderOptions);

    if ( renderOptions->drawOutlines ) {
        Opengl::openGlRenderSetColor(&renderOptions->outlineColor, renderOptions);
        Opengl::openGlRenderLine(&vertices[0], &vertices[1]);
        Opengl::openGlRenderLine(&vertices[1], &vertices[2]);
        Opengl::openGlRenderLine(&vertices[2], &vertices[0]);
    }
}

void
StochasticRadiosityOpenGLRenderer::renderQuadrilateral(
    const Vertex *v1,
    const Vertex *v2,
    const Vertex *v3,
    const Vertex *v4,
    const RenderOptions *renderOptions)
{
    ColorRgb colors[4];
    Vector3D vertices[4];

    vertices[0] = *(v1->point);
    colors[0] = v1->color;
    vertices[1] = *(v2->point);
    colors[1] = v2->color;
    vertices[2] = *(v3->point);
    colors[2] = v3->color;
    vertices[3] = *(v4->point);
    colors[3] = v4->color;
    Opengl::openGlRenderPolygonGouraud(4, vertices, colors, renderOptions);

    if ( renderOptions->drawOutlines ) {
        Opengl::openGlRenderSetColor(&renderOptions->outlineColor, renderOptions);
        Opengl::openGlRenderLine(&vertices[0], &vertices[1]);
        Opengl::openGlRenderLine(&vertices[1], &vertices[2]);
        Opengl::openGlRenderLine(&vertices[2], &vertices[3]);
        Opengl::openGlRenderLine(&vertices[3], &vertices[0]);
    }
}

void
StochasticRadiosityOpenGLRenderer::triangleTVertexElimination(
    Vertex **vertices,
    Vertex **midpoints,
    int numberOfTVertices,
    void (*doTriangleCallback)(const Vertex *, const Vertex *, const Vertex *, const RenderOptions *),
    const RenderOptions *renderOptions)
{
    int a;
    int b;
    int c;

    switch ( numberOfTVertices ) {
        case 0:
            doTriangleCallback(vertices[0], vertices[1], vertices[2], renderOptions);
            break;
        case 1:
            for ( a = 0; a < 3; a++ ) {
                if ( midpoints[a] ) {
                    break;
                }
            }
            b = (a + 1) % 3;
            c = (a + 2) % 3;
            doTriangleCallback(midpoints[a], vertices[c], vertices[a], renderOptions);
            doTriangleCallback(midpoints[a], vertices[b], vertices[c], renderOptions);
            break;
        case 2:
            for ( a = 0; a < 3; a++ ) {
                if ( !midpoints[a] ) {
                    break;
                }
            }
            b = (a + 1) % 3;
            c = (a + 2) % 3;
            doTriangleCallback(midpoints[b], vertices[a], vertices[b], renderOptions);
            doTriangleCallback(midpoints[b], midpoints[c], vertices[a], renderOptions);
            doTriangleCallback(midpoints[b], vertices[c], midpoints[c], renderOptions);
            break;
        case 3:
            doTriangleCallback(vertices[0], midpoints[0], midpoints[2], renderOptions);
            doTriangleCallback(vertices[1], midpoints[1], midpoints[0], renderOptions);
            doTriangleCallback(vertices[2], midpoints[2], midpoints[1], renderOptions);
            doTriangleCallback(midpoints[0], midpoints[1], midpoints[2], renderOptions);
            break;
        default:
            break;
    }
}

void
StochasticRadiosityOpenGLRenderer::quadrilateralTVertexElimination(
    Vertex **vertices,
    Vertex **midpoints,
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
            doQuadrilateralCallback(vertices[0], vertices[1], vertices[2], vertices[3], renderOptions);
            break;
        case 1:
            for ( a = 0; a < 4; a++ ) {
                if ( midpoints[a] ) {
                    break;
                }
            }
            b = (a + 1) % 4;
            c = (a + 2) % 4;
            d = (a + 3) % 4;
            doTriangleCallback(midpoints[a], vertices[d], vertices[a], renderOptions);
            doTriangleCallback(midpoints[a], vertices[c], vertices[d], renderOptions);
            doTriangleCallback(midpoints[a], vertices[b], vertices[c], renderOptions);
            break;
        case 2:
            for ( a = 0; a < 4; a++ ) {
                if ( midpoints[a] ) {
                    break;
                }
            }
            b = (a + 1) % 4;
            c = (a + 2) % 4;
            d = (a + 3) % 4;
            if ( midpoints[d] ) {
                a = d;
                b = (a + 1) % 4;
                c = (a + 2) % 4;
                d = (a + 3) % 4;
            }
            if ( midpoints[b] ) {
                doTriangleCallback(midpoints[a], vertices[b], midpoints[b], renderOptions);
                doTriangleCallback(midpoints[b], vertices[c], vertices[d], renderOptions);
                doTriangleCallback(vertices[d], midpoints[a], midpoints[b], renderOptions);
                doTriangleCallback(vertices[d], vertices[a], midpoints[a], renderOptions);
            } else {
                doQuadrilateralCallback(vertices[a], midpoints[a], midpoints[c], vertices[d], renderOptions);
                doQuadrilateralCallback(midpoints[a], vertices[b], vertices[c], midpoints[c], renderOptions);
            }
            break;
        case 3:
            for ( a = 0; a < 4; a++ ) {
                if ( !midpoints[a] ) {
                    break;
                }
            }
            b = (a + 1) % 4;
            c = (a + 2) % 4;
            d = (a + 3) % 4;
            doQuadrilateralCallback(vertices[a], vertices[b], midpoints[b], midpoints[d], renderOptions);
            doTriangleCallback(midpoints[b], vertices[c], midpoints[c], renderOptions);
            doTriangleCallback(midpoints[c], vertices[d], midpoints[d], renderOptions);
            doTriangleCallback(midpoints[b], midpoints[c], midpoints[d], renderOptions);
            break;
        case 4:
            doTriangleCallback(vertices[0], midpoints[0], midpoints[3], renderOptions);
            doTriangleCallback(vertices[1], midpoints[1], midpoints[0], renderOptions);
            doTriangleCallback(vertices[2], midpoints[2], midpoints[1], renderOptions);
            doTriangleCallback(vertices[3], midpoints[3], midpoints[2], renderOptions);
            doQuadrilateralCallback(midpoints[0], midpoints[1], midpoints[2], midpoints[3], renderOptions);
            break;
        default:
            break;
    }
}

void
StochasticRadiosityOpenGLRenderer::renderTriangularElement(
    Vertex **vertices,
    Vertex **midpoints,
    int numberOfTVertices,
    const RenderOptions *renderOptions)
{
    triangleTVertexElimination(vertices, midpoints, numberOfTVertices, renderTriangle, renderOptions);
}

void
StochasticRadiosityOpenGLRenderer::renderQuadrilateralElement(
    Vertex **vertices,
    Vertex **midpoints,
    int numberOfTVertices,
    const RenderOptions *renderOptions)
{
    quadrilateralTVertexElimination(vertices, midpoints, numberOfTVertices, renderTriangle, renderQuadrilateral, renderOptions);
}

void
StochasticRadiosityOpenGLRenderer::renderOutline(const StochasticRadiosityElement *element, const RenderOptions *renderOptions) {
    Vector3D vertices[4];
    vertices[0] = *(element->vertices[0]->point);
    vertices[1] = *(element->vertices[1]->point);
    vertices[2] = *(element->vertices[2]->point);
    if ( element->numberOfVertices > 3 ) {
        vertices[3] = *(element->vertices[3]->point);
    }

    Opengl::openGlRenderSetColor(&renderOptions->outlineColor, renderOptions);
    Opengl::openGlRenderLine(&vertices[0], &vertices[1]);
    Opengl::openGlRenderLine(&vertices[1], &vertices[2]);
    if ( element->numberOfVertices == 3 ) {
        Opengl::openGlRenderLine(&vertices[2], &vertices[0]);
    } else {
        Opengl::openGlRenderLine(&vertices[2], &vertices[3]);
        Opengl::openGlRenderLine(&vertices[3], &vertices[0]);
    }
}

void
StochasticRadiosityOpenGLRenderer::renderElement(Element *element, const RenderOptions *renderOptions) {
    StochasticRadiosityElement *stochasticElement = static_cast<StochasticRadiosityElement *>(element);
    Vector3D vertices[4];

    if ( renderOptions->smoothShading && GLOBAL_stochasticRaytracing_hierarchy.tvertex_elimination ) {
        Vertex *midpoints[4]{};
        int numberOfTVertices = 0;
        for ( int i = 0; i < stochasticElement->numberOfVertices; i++ ) {
            midpoints[i] = StochasticRadiosityElement::stochasticRadiosityElementEdgeMidpointVertex(stochasticElement, i);
            if ( midpoints[i] ) {
                numberOfTVertices++;
            }
        }

        if ( stochasticElement->numberOfVertices == 3 ) {
            renderTriangularElement(stochasticElement->vertices, midpoints, numberOfTVertices, renderOptions);
        } else {
            renderQuadrilateralElement(stochasticElement->vertices, midpoints, numberOfTVertices, renderOptions);
        }
        return;
    }

    vertices[0] = *(stochasticElement->vertices[0]->point);
    vertices[1] = *(stochasticElement->vertices[1]->point);
    vertices[2] = *(stochasticElement->vertices[2]->point);
    if ( stochasticElement->numberOfVertices > 3 ) {
        vertices[3] = *(stochasticElement->vertices[3]->point);
    }

    if ( renderOptions->smoothShading ) {
        ColorRgb vertexColors[4];
        vertexColors[0] = stochasticElement->vertices[0]->color;
        vertexColors[1] = stochasticElement->vertices[1]->color;
        vertexColors[2] = stochasticElement->vertices[2]->color;
        if ( stochasticElement->numberOfVertices > 3 ) {
            vertexColors[3] = stochasticElement->vertices[3]->color;
        }

        Opengl::openGlRenderPolygonGouraud(
            stochasticElement->numberOfVertices,
            vertices,
            vertexColors,
            renderOptions);
    } else {
        ColorRgb color = StochasticRadiosityElement::stochasticRadiosityElementColor(stochasticElement);
        Opengl::openGlRenderSetColor(&color, renderOptions);
        Opengl::openGlRenderPolygonFlat(stochasticElement->numberOfVertices, vertices);
    }

    if ( renderOptions->drawOutlines ) {
        renderOutline(stochasticElement, renderOptions);
    }
}

#endif
