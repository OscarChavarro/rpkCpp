/* render.c: rendering ELEMENTs */
/* TODO: mesh balancing */

#include "common/error.h"
#include "shared/camera.h"
#include "shared/render.h"
#include "IMAGE/tonemapping.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

RGB ElementColor(ELEMENT *element) {
    RGB color;

    switch ( mcr.show ) {
        case SHOW_TOTAL_RADIANCE:
        case SHOW_INDIRECT_RADIANCE:
            RadianceToRGB(ElementDisplayRadiance(element), &color);
            break;
        case SHOW_IMPORTANCE: {
            float gray = element->imp > 1. ? 1. : element->imp < 0. ? 0. : element->imp;
            RGBSET(color, gray, gray, gray);
            break;
        }
        default:
            Fatal(-1, "ElementColor", "Don't know what to display (mcr.show = %d)", mcr.show);
    }

    return color;
}

static COLOR VertexRadiance(VERTEX *v) {
    int count = 0;
    COLOR radiance;

    COLORCLEAR(radiance);
    ForAllElements(elem, v->radiance_data)
                {
                    if ( !elem->regular_subelements ) {
                        COLOR elemrad = ElementDisplayRadiance(elem);
                        COLORADD(radiance, elemrad, radiance);
                        count++;
                    }
                }
    EndForAll;

    if ( count > 0 ) COLORSCALEINVERSE((float) count, radiance, radiance);

    return radiance;
}

/* same as above but for the reflectance
 * (important in case of texturing) */
COLOR VertexReflectance(VERTEX *v) {
    int count = 0;
    COLOR rd;

    COLORCLEAR(rd);
    ForAllElementsSharingVertex(elem, v)
                {
                    if ( !elem->regular_subelements ) {
                        COLORADD(rd, elem->Rd, rd);
                        count++;
                    }
                }
    EndForAll;

    if ( count > 0 ) COLORSCALEINVERSE((float) count, rd, rd);

    return rd;
}

/* same as above but for importance */
static float VertexImportance(VERTEX *v) {
    int count = 0;
    float imp = 0.;
    ForAllElements(elem, v->radiance_data)
                {
                    if ( !elem->regular_subelements ) {
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

RGB VertexColor(VERTEX *v) {
    switch ( mcr.show ) {
        case SHOW_TOTAL_RADIANCE:
        case SHOW_INDIRECT_RADIANCE:
            RadianceToRGB(VertexRadiance(v), &v->color);
            break;
        case SHOW_IMPORTANCE: {
            float gray = VertexImportance(v);
            if ( gray > 1. ) {
                gray = 1.;
            }
            if ( gray < 0. ) {
                gray = 0.;
            }
            RGBSET(v->color, gray, gray, gray);
            break;
        }
        default:
            Fatal(-1, "VertexColor", "Don't know what to display (mcr.show = %d)", mcr.show);
    }

    return v->color;
}

/* compute new vertex colors */
void ElementComputeNewVertexColors(ELEMENT *elem) {
    VertexColor(elem->vertex[0]);
    VertexColor(elem->vertex[1]);
    VertexColor(elem->vertex[2]);
    if ( elem->nrvertices > 3 ) {
        VertexColor(elem->vertex[3]);
    }
}

void ElementAdjustTVertexColors(ELEMENT *elem) {
    VERTEX *m[4];
    int i, n;
    for ( i = 0, n = 0; i < elem->nrvertices; i++ ) {
        m[i] = EdgeMidpointVertex(elem, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( n > 0 ) {
        RGB color = ElementColor(elem);
        for ( i = 0; i < elem->nrvertices; i++ ) {
            if ( m[i] ) {
                m[i]->color.r = (m[i]->color.r + color.r) * 0.5;
                m[i]->color.g = (m[i]->color.g + color.g) * 0.5;
                m[i]->color.b = (m[i]->color.b + color.b) * 0.5;
            }
        }
    }
}

static void RenderTriangle(VERTEX *v1, VERTEX *v2, VERTEX *v3) {
    RGB col[3];
    Vector3D vert[3];

    vert[0] = *(v1->point);
    col[0] = v1->color;
    vert[1] = *(v2->point);
    col[1] = v2->color;
    vert[2] = *(v3->point);
    col[2] = v3->color;
    RenderPolygonGouraud(3, vert, col);

    if ( renderopts.draw_outlines ) {
        int i;
        for ( i = 0; i < 3; i++ ) {
            Vector3D d;
            VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyep, vert[i], d);
            VECTORSUMSCALED(vert[i], 0.01, d, vert[i]);
        }

        RenderSetColor(&renderopts.outline_color);
        RenderLine(&vert[0], &vert[1]);
        RenderLine(&vert[1], &vert[2]);
        RenderLine(&vert[2], &vert[0]);
    }
}

static void RenderQuadrilateral(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4) {
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
    RenderPolygonGouraud(4, vert, col);

    if ( renderopts.draw_outlines ) {
        int i;
        for ( i = 0; i < 4; i++ ) {
            Vector3D d;
            VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyep, vert[i], d);
            VECTORSUMSCALED(vert[i], 0.01, d, vert[i]);
        }

        RenderSetColor(&renderopts.outline_color);
        RenderLine(&vert[0], &vert[1]);
        RenderLine(&vert[1], &vert[2]);
        RenderLine(&vert[2], &vert[3]);
        RenderLine(&vert[3], &vert[0]);
    }
}

/* TODO: The T-vertex elimination code only fully eliminates T-vertices in
 * balanced meshes. Meshes are not yet balanced [PhB - 9901]. */

static void TriangleTVertexElimination(VERTEX **v, VERTEX **m, int nrTvertices,
                                       void (*do_triangle)(VERTEX *, VERTEX *, VERTEX *),
                                       void (*do_quadrilateral)(VERTEX *, VERTEX *, VERTEX *, VERTEX *)) {
    int a, b, c;

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

static void QuadrilateralTVertexElimination(VERTEX **v, VERTEX **m, int nrTvertices,
                                            void (*do_triangle)(VERTEX *, VERTEX *, VERTEX *),
                                            void (*do_quadrilateral)(VERTEX *, VERTEX *, VERTEX *, VERTEX *)) {
    int a, b, c, d;

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

static void RenderTriangularElement(VERTEX **v, VERTEX **m, int nrTvertices) {
    TriangleTVertexElimination(v, m, nrTvertices, RenderTriangle, RenderQuadrilateral);
}

static void RenderQuadrilateralElement(VERTEX **v, VERTEX **m, int nrTvertices) {
    QuadrilateralTVertexElimination(v, m, nrTvertices, RenderTriangle, RenderQuadrilateral);
}

void ElementTVertexElimination(ELEMENT *elem,
                               void (*do_triangle)(VERTEX *, VERTEX *, VERTEX *),
                               void (*do_quadrilateral)(VERTEX *, VERTEX *, VERTEX *, VERTEX *)) {
    VERTEX *m[4];
    int i, n;
    for ( i = 0, n = 0; i < elem->nrvertices; i++ ) {
        m[i] = EdgeMidpointVertex(elem, i);
        if ( m[i] ) {
            n++;
        }
    }

    if ( elem->nrvertices == 3 ) {
        TriangleTVertexElimination(elem->vertex, m, n, do_triangle, do_quadrilateral);
    } else {
        QuadrilateralTVertexElimination(elem->vertex, m, n, do_triangle, do_quadrilateral);
    }
}

void RenderElementOutline(ELEMENT *elem) {
    Vector3D verts[4];
    int i;

    /* test whether eye point is in front of the patch */
    if ( VECTORDOTPRODUCT(elem->pog.patch->normal, GLOBAL_camera_mainCamera.eyep) + elem->pog.patch->plane_constant < 0. ) {
        return;
    }

    for ( i = 0; i < elem->nrvertices; i++ ) {
        Vector3D d;
        verts[i] = *(elem->vertex[i]->point);
        VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyep, verts[i], d);
        VECTORSUMSCALED(verts[i], 0.0001, d, verts[i]);
    }

    RenderSetColor(&renderopts.outline_color);
    RenderLine(&verts[0], &verts[1]);
    RenderLine(&verts[1], &verts[2]);
    if ( elem->nrvertices == 3 ) {
        RenderLine(&verts[2], &verts[0]);
    } else {
        RenderLine(&verts[2], &verts[3]);
        RenderLine(&verts[3], &verts[0]);
    }
}

void RenderElement(ELEMENT *elem) {
    Vector3D verts[4];

    if ( renderopts.smooth_shading && hierarchy.tvertex_elimination ) {
        VERTEX *m[4];
        int i, n;
        for ( i = 0, n = 0; i < elem->nrvertices; i++ ) {
            m[i] = EdgeMidpointVertex(elem, i);
            if ( m[i] ) {
                n++;
            }
        }

        if ( elem->nrvertices == 3 ) {
            RenderTriangularElement(elem->vertex, m, n);
        } else {
            RenderQuadrilateralElement(elem->vertex, m, n);
        }
        return;
    }

    verts[0] = *(elem->vertex[0]->point);
    verts[1] = *(elem->vertex[1]->point);
    verts[2] = *(elem->vertex[2]->point);
    if ( elem->nrvertices > 3 ) {
        verts[3] = *(elem->vertex[3]->point);
    }

    if ( renderopts.smooth_shading ) {
        RGB vertcols[4];
        vertcols[0] = elem->vertex[0]->color;
        vertcols[1] = elem->vertex[1]->color;
        vertcols[2] = elem->vertex[2]->color;
        if ( elem->nrvertices > 3 ) {
            vertcols[3] = elem->vertex[3]->color;
        }

        RenderPolygonGouraud(elem->nrvertices, verts, vertcols);
    } else {
        RGB color = ElementColor(elem);

        RenderSetColor(&color);
        RenderPolygonFlat(elem->nrvertices, verts);
    }

    if ( renderopts.draw_outlines )
        RenderElementOutline(elem);
}

COLOR ElementDisplayRadiance(ELEMENT *elem) {
    COLOR rad;
    COLORSUBTRACT(elem->rad[0], elem->source_rad, rad);

    if ( mcr.show != SHOW_INDIRECT_RADIANCE ) {
        /* source_rad is self-emitted radiance if !mcr.indirect_only. It is direct
         * illumination if mcr.direct_only */
        COLORADD(rad, elem->source_rad, rad);
        if ( mcr.indirect_only || mcr.do_nondiffuse_first_shot ) {
            /* add self-emitted radiance */
            COLORADD(rad, elem->Ed, rad);
        }
    }
    return rad;
}

COLOR ElementDisplayRadianceAtPoint(ELEMENT *elem, double u, double v) {
    COLOR radiance;
    if ( elem->basis->size == 1 ) {
        if ( renderopts.smooth_shading ) {
            /* do Gouraud interpolation if required */
            int i;
            COLOR rad[4];
            for ( i = 0; i < elem->nrvertices; i++ ) {
                rad[i] = VertexRadiance(elem->vertex[i]);
            }
            switch ( elem->nrvertices ) {
                case 3: COLORINTERPOLATEBARYCENTRIC(rad[0], rad[1], rad[2], u, v, radiance);
                    break;
                case 4: COLORINTERPOLATEBILINEAR(rad[0], rad[1], rad[2], rad[3], u, v, radiance);
                    break;
                default:
                    Fatal(-1, "ElementDisplayRadianceAtPoint", "can only handle triangular or quadrilateral elements");
                    COLORCLEAR(radiance);
            }
        } else {    /* flat shading */
            radiance = ElementDisplayRadiance(elem);
        }
    } else {    /* higher order approximations */
        radiance = ColorAtUV(elem->basis, elem->rad, u, v);
        if ( mcr.show == SHOW_INDIRECT_RADIANCE ) {
            COLORSUBTRACT(radiance, elem->source_rad, radiance);
        }
    }
    return radiance;
}
