/**
Rendering elements
*/

#include "common/error.h"
#include "shared/render.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "render/opengl.h"

RGB
elementColor(StochasticRadiosityElement *element) {
    RGB color;

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show ) {
        case SHOW_TOTAL_RADIANCE:
        case SHOW_INDIRECT_RADIANCE:
            radianceToRgb(elementDisplayRadiance(element), &color);
            break;
        case SHOW_IMPORTANCE: {
            float gray = element->imp > 1.0 ? 1.0 : element->imp < 0.0 ? 0.0 : element->imp;
            setRGB(color, gray, gray, gray);
            break;
        }
        default:
            logFatal(-1, "elementColor",
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
    ForAllElements(elem, v->radiance_data)
                {
                    if ( !elem->regularSubElements ) {
                        COLOR elemrad = elementDisplayRadiance(elem);
                        colorAdd(radiance, elemrad, radiance);
                        count++;
                    }
                }
    EndForAll;

    if ( count > 0 ) {
        colorScaleInverse((float) count, radiance, radiance);
    }

    return radiance;
}

/**
Same as above but for the reflectance (important in case of texturing)
*/
COLOR
vertexReflectance(Vertex *v) {
    int count = 0;
    COLOR rd;

    colorClear(rd);
    ForAllElementsSharingVertex(elem, v)
                {
                    if ( !elem->regularSubElements ) {
                        colorAdd(rd, elem->Rd, rd);
                        count++;
                    }
                }
    EndForAll;

    if ( count > 0 ) {
        colorScaleInverse((float) count, rd, rd);
    }

    return rd;
}

/**
Same as above but for importance
*/
static float
vertexImportance(Vertex *v) {
    int count = 0;
    float imp = 0.;
    ForAllElements(elem, v->radiance_data)
                {
                    if ( !elem->regularSubElements ) {
                        imp += elem->imp;
                        count++;
                    }
                }
    EndForAll;

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
elementComputeNewVertexColors(StochasticRadiosityElement *elem) {
    vertexColor(elem->vertex[0]);
    vertexColor(elem->vertex[1]);
    vertexColor(elem->vertex[2]);
    if ( elem->numberOfVertices > 3 ) {
        vertexColor(elem->vertex[3]);
    }
}

void
elementAdjustTVertexColors(StochasticRadiosityElement *elem) {
    Vertex *m[4];
    int i, n;
    for ( i = 0, n = 0; i < elem->numberOfVertices; i++ ) {
        m[i] = monteCarloRadiosityEdgeMidpointVertex(elem, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( n > 0 ) {
        RGB color = elementColor(elem);
        for ( i = 0; i < elem->numberOfVertices; i++ ) {
            if ( m[i] ) {
                m[i]->color.r = (m[i]->color.r + color.r) * 0.5;
                m[i]->color.g = (m[i]->color.g + color.g) * 0.5;
                m[i]->color.b = (m[i]->color.b + color.b) * 0.5;
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

    if ( GLOBAL_render_renderOptions.draw_outlines ) {
        int i;
        for ( i = 0; i < 3; i++ ) {
            Vector3D d;
            VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyePosition, vert[i], d);
            VECTORSUMSCALED(vert[i], 0.01, d, vert[i]);
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

    if ( GLOBAL_render_renderOptions.draw_outlines ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            Vector3D d;
            VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyePosition, vert[i], d);
            VECTORSUMSCALED(vert[i], 0.01, d, vert[i]);
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
    int nrTvertices,
    void (*do_triangle)(Vertex *, Vertex *, Vertex *),
    void (*do_quadrilateral)(Vertex *, Vertex *, Vertex *, Vertex *))
{
    int a;
    int b;
    int c;

    switch ( nrTvertices ) {
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
    }
}

static void
quadrilateralTVertexElimination(
    Vertex **v,
    Vertex **m,
    int nrTvertices,
    void (*do_triangle)(Vertex *, Vertex *, Vertex *),
    void (*do_quadrilateral)(Vertex *, Vertex *, Vertex *, Vertex *))
{
    int a;
    int b;
    int c;
    int d;

    switch ( nrTvertices ) {
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
    }
}

static void
renderTriangularElement(Vertex **v, Vertex **m, int nrTvertices) {
    triangleTVertexElimination(v, m, nrTvertices, renderTriangle, renderQuadrilateral);
}

static void
renderQuadrilateralElement(Vertex **v, Vertex **m, int nrTvertices) {
    quadrilateralTVertexElimination(v, m, nrTvertices, renderTriangle, renderQuadrilateral);
}

void
elementTVertexElimination(
    StochasticRadiosityElement *elem,
    void (*do_triangle)(Vertex *, Vertex *, Vertex *),
    void (*do_quadrilateral)(Vertex *, Vertex *, Vertex *, Vertex *))
{
    Vertex *m[4];
    int i, n;
    for ( i = 0, n = 0; i < elem->numberOfVertices; i++ ) {
        m[i] = monteCarloRadiosityEdgeMidpointVertex(elem, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( elem->numberOfVertices == 3 ) {
        triangleTVertexElimination(elem->vertex, m, n, do_triangle, do_quadrilateral);
    } else {
        quadrilateralTVertexElimination(elem->vertex, m, n, do_triangle, do_quadrilateral);
    }
}

void
renderElementOutline(StochasticRadiosityElement *elem) {
    Vector3D verts[4];
    int i;

    /* test whether eye point is in front of the patch */
    if ( VECTORDOTPRODUCT(elem->patch->normal, GLOBAL_camera_mainCamera.eyePosition) + elem->patch->planeConstant < 0. ) {
        return;
    }

    for ( i = 0; i < elem->numberOfVertices; i++ ) {
        Vector3D d;
        verts[i] = *(elem->vertex[i]->point);
        VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyePosition, verts[i], d);
        VECTORSUMSCALED(verts[i], 0.0001, d, verts[i]);
    }

    openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
    openGlRenderLine(&verts[0], &verts[1]);
    openGlRenderLine(&verts[1], &verts[2]);
    if ( elem->numberOfVertices == 3 ) {
        openGlRenderLine(&verts[2], &verts[0]);
    } else {
        openGlRenderLine(&verts[2], &verts[3]);
        openGlRenderLine(&verts[3], &verts[0]);
    }
}

void
mcrRenderElement(StochasticRadiosityElement *elem) {
    Vector3D verts[4];

    if ( GLOBAL_render_renderOptions.smooth_shading && GLOBAL_stochasticRaytracing_hierarchy.tvertex_elimination ) {
        Vertex *m[4];
        int i, n;
        for ( i = 0, n = 0; i < elem->numberOfVertices; i++ ) {
            m[i] = monteCarloRadiosityEdgeMidpointVertex(elem, i);
            if ( m[i] ) {
                n++;
            }
        }

        if ( elem->numberOfVertices == 3 ) {
            renderTriangularElement(elem->vertex, m, n);
        } else {
            renderQuadrilateralElement(elem->vertex, m, n);
        }
        return;
    }

    verts[0] = *(elem->vertex[0]->point);
    verts[1] = *(elem->vertex[1]->point);
    verts[2] = *(elem->vertex[2]->point);
    if ( elem->numberOfVertices > 3 ) {
        verts[3] = *(elem->vertex[3]->point);
    }

    if ( GLOBAL_render_renderOptions.smooth_shading ) {
        RGB vertcols[4];
        vertcols[0] = elem->vertex[0]->color;
        vertcols[1] = elem->vertex[1]->color;
        vertcols[2] = elem->vertex[2]->color;
        if ( elem->numberOfVertices > 3 ) {
            vertcols[3] = elem->vertex[3]->color;
        }

        openGlRenderPolygonGouraud(elem->numberOfVertices, verts, vertcols);
    } else {
        RGB color = elementColor(elem);

        openGlRenderSetColor(&color);
        openGlRenderPolygonFlat(elem->numberOfVertices, verts);
    }

    if ( GLOBAL_render_renderOptions.draw_outlines )
        renderElementOutline(elem);
}

COLOR
elementDisplayRadiance(StochasticRadiosityElement *elem) {
    COLOR rad;
    colorSubtract(elem->rad[0], elem->sourceRad, rad);

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
elementDisplayRadianceAtPoint(StochasticRadiosityElement *elem, double u, double v) {
    COLOR radiance;
    if ( elem->basis->size == 1 ) {
        if ( GLOBAL_render_renderOptions.smooth_shading ) {
            // Do Gouraud interpolation if required
            int i;
            COLOR rad[4];
            for ( i = 0; i < elem->numberOfVertices; i++ ) {
                rad[i] = vertexRadiance(elem->vertex[i]);
            }
            switch ( elem->numberOfVertices ) {
                case 3:
                    colorInterpolateBarycentric(rad[0], rad[1], rad[2], u, v, radiance);
                    break;
                case 4:
                    colorInterpolateBilinear(rad[0], rad[1], rad[2], rad[3], u, v, radiance);
                    break;
                default:
                    logFatal(-1, "elementDisplayRadianceAtPoint",
                             "can only handle triangular or quadrilateral elements");
                    colorClear(radiance);
            }
        } else {
            // Flat shading
            radiance = elementDisplayRadiance(elem);
        }
    } else {
        // Higher order approximations
        radiance = colorAtUv(elem->basis, elem->rad, u, v);
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show == SHOW_INDIRECT_RADIANCE ) {
            colorSubtract(radiance, elem->sourceRad, radiance);
        }
    }
    return radiance;
}
