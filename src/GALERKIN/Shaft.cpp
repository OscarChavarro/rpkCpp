#include "java/util/ArrayList.txx"
#include "GALERKIN/Shaft.h"

#define MIN_MAX_DIMENSIONS 6

Shaft::Shaft():
    referenceItem1(),
    referenceItem2(),
    extentBoundingBox(),
    planeSet(),
    numberOfPlanesInSet(),
    omit(),
    numberOfGeometriesToOmit(),
    dontOpen(),
    numberOfGeometriesToNotOpen(),
    center1(),
    center2(),
    cut()
{
}

/**
Marks a geometry as to be omitted during shaft culling: it will not be added to the
candidate list, even if the geometry overlaps or is inside the shaft
*/
void
Shaft::setShaftOmit(Patch *patch) {
    omit[numberOfGeometriesToOmit++] = patch;
}

/**
Marks a geometry as one not to be opened during shaft culling
*/
void
Shaft::setShaftDontOpen(Geometry *geometry) {
    dontOpen[numberOfGeometriesToNotOpen++] = geometry;
}

/**
Constructs a shaft for two given bounding boxes
*/
void
Shaft::constructFromBoundingBoxes(BoundingBox *boundingBox1, BoundingBox *boundingBox2) {
    numberOfGeometriesToOmit = 0;
    numberOfGeometriesToNotOpen = 0;
    cut = false;

    // 1. Obtain the bounding boxes for the reference items [HAIN1991]
    referenceItem1 = boundingBox1;
    referenceItem2 = boundingBox2;

    // Midpoints of the reference boxes define a line that is guaranteed
    // to lay within the shaft
    center1.x = 0.5f * (boundingBox1->coordinates[MIN_X] + boundingBox1->coordinates[MAX_X]);
    center1.y = 0.5f * (boundingBox1->coordinates[MIN_Y] + boundingBox1->coordinates[MAX_Y]);
    center1.z = 0.5f * (boundingBox1->coordinates[MIN_Z] + boundingBox1->coordinates[MAX_Z]);

    center2.x = 0.5f * (boundingBox2->coordinates[MIN_X] + boundingBox2->coordinates[MAX_X]);
    center2.y = 0.5f * (boundingBox2->coordinates[MIN_Y] + boundingBox2->coordinates[MAX_Y]);
    center2.z = 0.5f * (boundingBox2->coordinates[MIN_Z] + boundingBox2->coordinates[MAX_Z]);

    // 2. Compute the extent bounding box containing both reference items [HAIN1991]
    bool hasMinMax1[MIN_MAX_DIMENSIONS]; // Representation of culled edges for the extent bounding box
    bool hasMinMax2[MIN_MAX_DIMENSIONS];

    for ( int i = 0; i < MIN_MAX_DIMENSIONS; i++ ) {
        hasMinMax1[i] = false;
        hasMinMax2[i] = false;
    }

    for ( int dimension = MIN_X; dimension <= MIN_Z; dimension++ ) {
        if ( referenceItem1->coordinates[dimension] < referenceItem2->coordinates[dimension] ) {
            extentBoundingBox.coordinates[dimension] = referenceItem1->coordinates[dimension];
            hasMinMax1[dimension] = true;
        } else {
            extentBoundingBox.coordinates[dimension] = referenceItem2->coordinates[dimension];
            if ( !doubleEqual(referenceItem1->coordinates[dimension], referenceItem2->coordinates[dimension], EPSILON) ) {
                hasMinMax2[dimension] = true;
            }
        }
    }

    for ( int dimension = MAX_X; dimension <= MAX_Z; dimension++ ) {
        if ( referenceItem1->coordinates[dimension] > referenceItem2->coordinates[dimension] ) {
            extentBoundingBox.coordinates[dimension] = referenceItem1->coordinates[dimension];
            hasMinMax1[dimension] = true;
        } else {
            extentBoundingBox.coordinates[dimension] = referenceItem2->coordinates[dimension];
            if ( !doubleEqual(referenceItem1->coordinates[dimension], referenceItem2->coordinates[dimension], EPSILON) ) {
                hasMinMax2[dimension] = true;
            }
        }
    }

    // 3. Create the plane set between the two reference items' boxes [HAIN1991]
    numberOfPlanesInSet = 0;
    ShaftPlane *localPlane = &planeSet[0];
    for ( int i = 0; i < MIN_MAX_DIMENSIONS; i++ ) {
        if ( !hasMinMax1[i] ) {
            continue;
        }

        for ( int j = 0; j < MIN_MAX_DIMENSIONS; j++ ) {
            // 3.1. Compute plane normal for marked borders
            int a = (i % 3); // Directions
            int b = (j % 3);

            if ( !hasMinMax2[j] || a == b ) {
                // Same direction
                continue;
            }

            float u1 = referenceItem1->coordinates[i]; // Coordinates defining the plane
            float v1 = referenceItem1->coordinates[j];
            float u2 = referenceItem2->coordinates[i];
            float v2 = referenceItem2->coordinates[j];
            float du;
            float dv;

            if ( (i <= MIN_Z && j <= MIN_Z) || (i >= MAX_X && j >= MAX_X) ) {
                du = v2 - v1;
                dv = u1 - u2;
            } else {
                // Normal must point outwards shaft
                du = v1 - v2;
                dv = u2 - u1;
            }

            // 3.2. Build the new identified plane
            localPlane->n[a] = du;
            localPlane->n[b] = dv;
            localPlane->n[3 - a - b] = 0.0;
            localPlane->d = -(du * u1 + dv * v1);
            localPlane->coordinateOffset[0] = localPlane->n[0] > 0.0 ? MIN_X : MAX_X;
            localPlane->coordinateOffset[1] = localPlane->n[1] > 0.0 ? MIN_Y : MAX_Y;
            localPlane->coordinateOffset[2] = localPlane->n[2] > 0.0 ? MIN_Z : MAX_Z;

            localPlane++;
        }
    }
    numberOfPlanesInSet = (int)(localPlane - &planeSet[0]);
}

/**
Tests a polygon with respect to the plane defined by the given normal and plane
constant. Returns INSIDE if the polygon is totally on the negative side of
the plane, OUTSIDE if the polygon on all on the positive side, OVERLAP
if the polygon is cut by the plane and COPLANAR if the polygon lays on the
plane within tolerance distance d*EPSILON
*/
ShaftPlanePosition
Shaft::testPolygonWithRespectToPlane(const Polygon *poly, const Vector3D *normal, const double d) {
    bool out; // out = there are positions on the positive side of the plane
    bool in; // in  = there are positions on the negative side of the plane

    out = false;
    in = false;
    for ( int i = 0; i < poly->numberOfVertices; i++ ) {
        double e = vectorDotProduct(*normal, poly->vertex[i]) + d;
        double tolerance = std::fabs(d) * EPSILON + poly->vertex[i].tolerance(EPSILON_FLOAT);
        out |= (e > tolerance);
        in |= (e < -tolerance);
        if ( out && in ) {
            return ShaftPlanePosition::OVERLAP;
        }
    }

    if ( out ) {
        return ShaftPlanePosition::OUTSIDE;
    } else {
        return in ? ShaftPlanePosition::INSIDE : ShaftPlanePosition::COPLANAR;
    }
}

/**
Verifies whether the polygon is on the given side of the plane. Returns true if
so, and false if not
*/
bool
Shaft::verifyPolygonWithRespectToPlane(const Polygon *polygon, const Vector3D *normal, const double d, const int side) {
    bool out = false;
    bool in = false;

    for ( int i = 0; i < polygon->numberOfVertices; i++ ) {
        double e = vectorDotProduct(*normal, polygon->vertex[i]) + d;
        double tolerance = std::fabs(d) * EPSILON + polygon->vertex[i].tolerance(EPSILON_FLOAT);
        out |= e > tolerance;
        if ( out && (side == ShaftPlanePosition::INSIDE || side == ShaftPlanePosition::COPLANAR) ) {
            return false;
        }
        in |= e < -tolerance;
        if ( in && (side == ShaftPlanePosition::OUTSIDE || side == ShaftPlanePosition::COPLANAR || (out && side != ShaftPlanePosition::OVERLAP)) ) {
            return false;
        }
    }

    if ( in ) {
        if ( out && side == ShaftPlanePosition::OVERLAP ) {
            return true;
        } else {
            if ( side == ShaftPlanePosition::INSIDE ) {
                return true;
            }
        }
    } else {
        if ( out ) {
            if ( side == ShaftPlanePosition::OUTSIDE ) {
                return true;
            }
        } else {
            if ( side == ShaftPlanePosition::COPLANAR ) {
                return true;
            }
        }
    }
    return false;
}

/**
Tests the position of a point with respect to a plane. Returns OUTSIDE if the point is
on the positive side of the plane, INSIDE if on the negative side, and COPLANAR
if the point is on the plane within tolerance distance d*EPSILON
*/
ShaftPlanePosition
Shaft::testPointWithRespectToPlane(const Vector3D *p, const Vector3D *normal, double d) {
    double e;
    double tolerance = std::fabs(d * EPSILON) + p->tolerance(EPSILON_FLOAT);
    e = vectorDotProduct(*normal, *p) + d;
    if ( e < -tolerance ) {
        return ShaftPlanePosition::INSIDE;
    }
    if ( e > +tolerance ) {
        return ShaftPlanePosition::OUTSIDE;
    }
    return ShaftPlanePosition::COPLANAR;
}

/**
Compare to shaft planes. Returns 0 if they are the same and -1 or +1
if not (can be used for sorting the planes. It is assumed that the plane normals
are normalized!
*/
int
Shaft::compareShaftPlanes(const ShaftPlane *plane1, const ShaftPlane *plane2) {
    double tolerance;

    // Compare components of plane normal (normalized vector, so components
    // are in the range [-1,1]
    if ( plane1->n[0] < plane2->n[0] - EPSILON ) {
        return -1;
    } else if ( plane1->n[0] > plane2->n[0] + EPSILON ) {
        return +1;
    }

    if ( plane1->n[1] < plane2->n[1] - EPSILON ) {
        return -1;
    } else if ( plane1->n[1] > plane2->n[1] + EPSILON ) {
        return +1;
    }

    if ( plane1->n[2] < plane2->n[2] - EPSILON ) {
        return -1;
    } else if ( plane1->n[2] > plane2->n[2] + EPSILON ) {
        return +1;
    }

    // Compare plane constants
    tolerance = std::fabs(floatMax(plane1->d, plane2->d) * EPSILON);
    if ( plane1->d < plane2->d - tolerance ) {
        return -1;
    } else if ( plane1->d > plane2->d + tolerance ) {
        return +1;
    }
    return 0;
}

/**
Plane is a pointer to the shaft plane defined in shaft. This routine will return
true if the plane differs from all previous defined planes
*/
int
Shaft::uniqueShaftPlane(const ShaftPlane *parameterPlane) const {
    for ( const ShaftPlane *ref = &planeSet[0]; ref != parameterPlane; ref++ ) {
        if ( compareShaftPlanes(ref, parameterPlane) == 0 ) {
            return false;
        }
    }
    return true;
}

/**
Fills in normal and plane constant, as will as the coord_offset parameters
*/
void
Shaft::fillInPlane(ShaftPlane *plane, float nx, float ny, float nz, float d) {
    plane->n[0] = nx;
    plane->n[1] = ny;
    plane->n[2] = nz;
    plane->d = d;

    plane->coordinateOffset[0] = plane->n[0] > 0.0 ? MIN_X : MAX_X;
    plane->coordinateOffset[1] = plane->n[1] > 0.0 ? MIN_Y : MAX_Y;
    plane->coordinateOffset[2] = plane->n[2] > 0.0 ? MIN_Z : MAX_Z;
}

/**
Construct the planes determining the shaft that use edges of p1 and vertices of p2
*/
void
Shaft::constructPolygonToPolygonPlanes(const Polygon *polygon1, const Polygon *polygon2) {
    Vector3D normal;
    ShaftPlane *localPlane = &planeSet[numberOfPlanesInSet];
    int maxPlanesPerEdge;

    // Test p2 wrt plane of p1
    normal.copy(polygon1->normal); // Convert to double precision
    switch ( testPolygonWithRespectToPlane(polygon2, &normal, polygon1->planeConstant) ) {
        case ShaftPlanePosition::INSIDE:
            // Polygon p2 is on the negative side of the plane of p1. The plane of p1 is
            // a shaft plane and there will be at most one shaft plane per edge of p1
            fillInPlane(localPlane, polygon1->normal.x, polygon1->normal.y, polygon1->normal.z, polygon1->planeConstant);
            if ( uniqueShaftPlane(localPlane) ) {
                localPlane++;
            }
            maxPlanesPerEdge = 1;
            break;
        case ShaftPlanePosition::OUTSIDE:
            // Like above, except that p2 is on the positive side of the plane of p1, so
            // we have to invert normal and plane constant
            fillInPlane(localPlane, -polygon1->normal.x, -polygon1->normal.y, -polygon1->normal.z, -polygon1->planeConstant);
            if ( uniqueShaftPlane(localPlane) ) {
                localPlane++;
            }
            maxPlanesPerEdge = 1;
            break;
        case ShaftPlanePosition::OVERLAP:
            // The plane of p1 cuts p2. It is not a shaft plane and there may be up to two
            // shaft planes for each edge of p1
            maxPlanesPerEdge = 2;
            break;
        default:
            // COPLANAR
            // Degenerate case that should never happen. If it does, it will result
            // in a shaft with no planes and just a thin bounding box containing the coplanar
            // faces
            return;
    }

    const Vector3D *cur;
    const Vector3D *next;
    const Vector3D *other;
    float d;
    float norm;
    ShaftPlanePosition side;
    int planesFoundForEdge;

    for ( int i = 0; i < polygon1->numberOfVertices; i++ ) {
        // For each edge of p1
        cur = &polygon1->vertex[i];
        next = &polygon1->vertex[(i + 1) % polygon1->numberOfVertices];

        planesFoundForEdge = 0;
        for ( int j = 0; j < polygon2->numberOfVertices && planesFoundForEdge < maxPlanesPerEdge; j++ ) {
            // For each vertex of p2
            other = &polygon2->vertex[j];

            // Compute normal and plane constant of the plane formed by cur, next and other
            vectorTripleCrossProduct(*cur, *next, *other, normal);
            norm = vectorNorm(normal);
            if ( norm < EPSILON ) {
                continue;
            }
            // Co-linear vertices, try next vertex on p2
            vectorScaleInverse(norm, normal, normal);
            d = -vectorDotProduct(normal, *cur);

            // Test position of p1 with respect to the constructed plane. Skip the vertices
            // that were used to construct the plane
            side = testPointWithRespectToPlane(&polygon1->vertex[(i + 2) % polygon1->numberOfVertices], &normal, d);
            for ( int k = (i + 3) % polygon1->numberOfVertices; k != i; k = (k + 1) % polygon1->numberOfVertices ) {
                ShaftPlanePosition nSide = testPointWithRespectToPlane(&polygon1->vertex[k], &normal, d);
                if ( side == ShaftPlanePosition::COPLANAR ) {
                    side = nSide;
                } else if ( nSide != ShaftPlanePosition::COPLANAR && side != nSide ) {
                    // side==INSIDE and nSide==OUTSIDE or vice versa
                    side = ShaftPlanePosition::OVERLAP;
                }
                // Or side==OVERLAP already. May happen due to round
            } // Off error e.g.
            if ( side != ShaftPlanePosition::INSIDE && side != ShaftPlanePosition::OUTSIDE ) {
                continue;
            } // Not a valid candidate shaft plane

            // Verify whether p2 is on the same side of the constructed plane. If so,
            // the plane is a candidate shaft plane and will be added to the list if
            // it is unique
            if ( verifyPolygonWithRespectToPlane(polygon2, &normal, d, side) ) {
                if ( side == ShaftPlanePosition::INSIDE ) {
                    // p1 and p2 are on the negative side as it should be
                    fillInPlane(localPlane, normal.x, normal.y, normal.z, d);
                } else {
                    fillInPlane(localPlane, -normal.x, -normal.y, -normal.z, -d);
                }
                if ( uniqueShaftPlane(localPlane) ) {
                    localPlane++;
                }
                planesFoundForEdge++;
            }
        }
    }

    numberOfPlanesInSet = (int)(localPlane - &planeSet[0]);
}

/**
Constructs a shaft enclosing the two given polygons
*/
void
Shaft::constructFromPolygonToPolygon(const Polygon *polygon1, const Polygon *polygon2) {
    // No "reference" bounding boxes to test with
    referenceItem1 = nullptr;
    referenceItem2 = nullptr;

    // Shaft extent: bounding box containing the bounding boxes of the patches
    extentBoundingBox.copyFrom(&polygon1->bounds);
    extentBoundingBox.enlarge(&polygon2->bounds);

    // Nothing (yet) to omit
    omit[0] = nullptr;
    omit[1] = nullptr;
    dontOpen[0] = nullptr;
    dontOpen[1] = nullptr;
    numberOfGeometriesToOmit = 0;
    numberOfGeometriesToNotOpen = 0;
    cut = false;

    // Center positions of polygons define a line that is guaranteed to lay inside the shaft
    center1 = polygon1->vertex[0];
    for ( int i = 1; i < polygon1->numberOfVertices; i++ ) {
        center1.addition(center1, polygon1->vertex[i]);
    }
    vectorScaleInverse((float) polygon1->numberOfVertices, center1, center1);

    center2 = polygon2->vertex[0];
    for ( int i = 1; i < polygon2->numberOfVertices; i++ ) {
        center2.addition(center2, polygon2->vertex[i]);
    }
    vectorScaleInverse((float) polygon2->numberOfVertices, center2, center2);

    // Determine the shaft planes
    numberOfPlanesInSet = 0;
    constructPolygonToPolygonPlanes(polygon1, polygon2);
    constructPolygonToPolygonPlanes(polygon2, polygon1);
}

/**
Tests a bounding volume against the shaft: returns INSIDE if the bounding volume
is inside the shaft, OVERLAP if it overlaps, OUTSIDE if it is outside the shaft
*/
ShaftPlanePosition
Shaft::boundingBoxTest(const BoundingBox *parameterBoundingBox) const {
    // Test against extent box
    if ( parameterBoundingBox->disjointToOtherBoundingBox(&extentBoundingBox) ) {
        return ShaftPlanePosition::OUTSIDE;
    }

    // Test against plane set: if nearest corner of the bounding box is on or
    // outside any shaft plane, the object is outside the shaft
    for ( int i = 0; i < numberOfPlanesInSet; i++ ) {
        const ShaftPlane *localPlane = &planeSet[i];
        if ( localPlane->n[0] * parameterBoundingBox->coordinates[localPlane->coordinateOffset[0]] +
             localPlane->n[1] * parameterBoundingBox->coordinates[localPlane->coordinateOffset[1]] +
             localPlane->n[2] * parameterBoundingBox->coordinates[localPlane->coordinateOffset[2]] +
             localPlane->d > -std::fabs(localPlane->d * EPSILON) ) {
            return ShaftPlanePosition::OUTSIDE;
        }
    }

    // Test against reference volumeListsOfItems
    if ((referenceItem1 && !parameterBoundingBox->disjointToOtherBoundingBox(referenceItem1)) ||
        (referenceItem2 && !parameterBoundingBox->disjointToOtherBoundingBox(referenceItem2)) ) {
        return ShaftPlanePosition::OVERLAP;
    }

    // If the bounding box survives all previous tests, it must overlap or be inside the
    // shaft. If the farthest corner of the bounding box is outside any shaft-plane, it
    // overlaps the shaft, otherwise it is inside the shaft
    for ( int i = 0; i < numberOfPlanesInSet; i++ ) {
        const ShaftPlane *localPlane = &planeSet[i];
        if ( localPlane->n[0] * parameterBoundingBox->coordinates[(localPlane->coordinateOffset[0] + 3) % 6] +
             localPlane->n[1] * parameterBoundingBox->coordinates[(localPlane->coordinateOffset[1] + 3) % 6] +
             localPlane->n[2] * parameterBoundingBox->coordinates[(localPlane->coordinateOffset[2] + 3) % 6] +
             localPlane->d > std::fabs(localPlane->d * EPSILON) ) {
            return ShaftPlanePosition::OVERLAP;
        }
    }

    return ShaftPlanePosition::INSIDE;
}

/**
Tests the patch against the shaft: returns INSIDE, OVERLAP or OUTSIDE according
to whether the patch is fully inside the shaft, overlapping it, or fully outside. If it is detected that the patch fully occludes the shaft, shaft->cut is
set to true, indicating that there is full occlusion due to one patch and
that as such no further shaft culling is necessary
*/
ShaftPlanePosition
Shaft::shaftPatchTest(Patch *patch) {
    int someOut;
    int inAll[MAXIMUM_VERTICES_PER_PATCH];
    const ShaftPlane *localPlane;
    double tMin[MAXIMUM_VERTICES_PER_PATCH];
    double tMax[MAXIMUM_VERTICES_PER_PATCH];
    double pTol[MAXIMUM_VERTICES_PER_PATCH];
    Ray ray;
    float dist;
    RayHit hitStore;

    // Start by assuming that all vertices are on the negative side ("inside") all shaft planes
    someOut = false;
    for ( int j = 0; j < patch->numberOfVertices; j++ ) {
        inAll[j] = true;
        tMin[j] = 0.0;  // Defines the segment of the edge that lays within the shaft
        tMax[j] = 1.0;
        pTol[j] = patch->vertex[j]->point->tolerance(EPSILON_FLOAT); // Vertex tolerance
    }

    localPlane = &planeSet[0];
    for ( int i = 0; i < numberOfPlanesInSet; i++ ) {
        // Test patch against i-th plane of the shaft
        Vector3D planeNormal;
        double e[MAXIMUM_VERTICES_PER_PATCH];
        double tolerance;
        int in;
        int out;
        int side[MAXIMUM_VERTICES_PER_PATCH];

        planeNormal.set(localPlane->n[0], localPlane->n[1], localPlane->n[2]);

        in = out = false;
        for ( int j = 0; j < patch->numberOfVertices; j++ ) {
            e[j] = vectorDotProduct(planeNormal, *patch->vertex[j]->point) + localPlane->d;
            tolerance = (float)(std::fabs(localPlane->d) * EPSILON + pTol[j]);
            side[j] = ShaftPlanePosition::COPLANAR;
            if ( e[j] > tolerance ) {
                side[j] = ShaftPlanePosition::OUTSIDE;
                out = true;
            } else if ( e[j] < -tolerance ) {
                side[j] = ShaftPlanePosition::INSIDE;
                in = true;
            }
            if ( side[j] != ShaftPlanePosition::INSIDE ) {
                inAll[j] = false;
            }
        }

        if ( !in ) {
            // Patch contains no vertices on the inside of the plane
            return ShaftPlanePosition::OUTSIDE;
        }

        if ( out ) {
            // Patch contains at least one vertex inside and one
            someOut = true; // Outside the plane. A point is inside the shaft
			 // if it is inside *all* planes, but it is outside
			 // as soon it is outside *one* plane

            for ( int j = 0; j < patch->numberOfVertices; j++ ) {
                // Reduce segment of edge that can lay within the shaft
                int k = (j + 1) % patch->numberOfVertices;
                if ( side[j] != side[k] ) {
                    if ( side[k] == ShaftPlanePosition::OUTSIDE ) {
                        // Decrease tMax[j]
                        if ( side[j] == ShaftPlanePosition::INSIDE ) {
                            // Find intersection
                            if ( tMax[j] > tMin[j] ) {
                                double t = e[j] / (e[j] - e[k]);
                                if ( t < tMax[j] ) {
                                    tMax[j] = t;
                                }
                            }
                        } else /* if (side[j] == COPLANAR) */ {
                            // Whole edge lays outside
                            tMax[j] = -EPSILON;
                        }
                    } else if ( side[j] == ShaftPlanePosition::OUTSIDE ) {
                        // increase tMin[j]
                        if ( side[k] == ShaftPlanePosition::INSIDE ) {
                            // Find intersection
                            if ( tMin[j] < tMax[j] ) {
                                double t = e[j] / (e[j] - e[k]);
                                if ( t > tMin[j] ) {
                                    tMin[j] = t;
                                }
                            }
                        } else /* if (side[k] == COPLANAR) */ {
                            // Whole edge lays outside
                            tMin[j] = 1. + EPSILON;
                        }
                    }
                } else if ( side[j] == ShaftPlanePosition::OUTSIDE ) {
                    // Whole edge lays outside
                    tMax[j] = -EPSILON;
                }
            }
        }
        localPlane++;
    }

    // The remaining tests only work if the shaft planes alone determine the shaft
    if ( referenceItem1 || referenceItem2 ) {
        return ShaftPlanePosition::OVERLAP;
    }

    if ( !someOut ) {
        // No patch vertices are outside the shaft
        return ShaftPlanePosition::INSIDE;
    }

    for ( int j = 0; j < patch->numberOfVertices; j++ ) {
        if ( inAll[j] ) {
            // At least one patch vertex is really inside the shaft and
            return ShaftPlanePosition::OVERLAP;
        }
    }
    // At least one vertex is really outside => overlap

    // All vertices are outside or on the shaft. Check whether there are edges
    // intersecting the shaft
    for ( int j = 0; j < patch->numberOfVertices; j++ ) {
        if ( tMin[j] + EPSILON < tMax[j] - EPSILON ) {
            return ShaftPlanePosition::OVERLAP;
        }
    }

    // All vertices and edges of the patch are outside the shaft. Either the
    // patch is totally outside the shaft, either, the patch cuts the shaft.
    // If the line segment connecting the midpoints of the polygons defining
    // the shaft intersects the patch, the patch cuts the shaft. If not,
    // the patch lays fully outside
    ray.pos = center1;
    ray.dir.subtraction(center2, center1);
    dist = 1.0f - EPSILON_FLOAT;
    if ( patch->intersect(&ray, EPSILON_FLOAT, &dist, HIT_FRONT | HIT_BACK, &hitStore) ) {
        cut = true;
        return ShaftPlanePosition::OVERLAP;
    }

    return ShaftPlanePosition::OUTSIDE;
}

/**
Returns true if the geometry is not to be enclosed in the shaft
*/
int
Shaft::patchIsOnOmitSet(const Patch *geometry) const {
    for ( int i = 0; i < numberOfGeometriesToOmit && i < MAX_SKIP_ELEMENTS; i++ ) {
        if ( omit[i] == geometry ) {
            return true;
        }
    }
    return false;
}

/**
Returns true if the geometry is not to be opened during shaft culling
*/
bool
Shaft::closedGeometry(const Geometry *geometry) const {
    for ( int i = 0; i < numberOfGeometriesToNotOpen && i < MAX_SKIP_ELEMENTS; i++ ) {
        if ( dontOpen[i] == geometry ) {
            return true;
        }
    }
    return false;
}

/**
Given a patchList on the shaft, this method will check every patch in patchList to
see if it is inside, outside or overlapping the shaft. Inside or overlapping patches
are added to culledPatchList. A pointer to the possibly elongated culledPatchList
is returned
*/
java::ArrayList<Patch *> *
Shaft::cullPatches(const java::ArrayList<Patch *> *patchList) {
    java::ArrayList<Patch *> *culledPatchList = new java::ArrayList<Patch *>();

    for ( int i = 0; patchList != nullptr && i < patchList->size() && !cut; i++ ) {
        Patch *patch = patchList->get(i);
        if ( patch->omit || patchIsOnOmitSet(patch) ) {
            continue;
        }

        if ( patch->boundingBox == nullptr ) {
            patch->computeBoundingBox();
        }

        ShaftPlanePosition boundingBoxSide = boundingBoxTest(patch->boundingBox);
        // Patch bounding box is inside the shaft, or overlaps with it. If it
        // overlaps, do a more expensive, but definitive, test to see whether
        // the patch itself is inside, outside or overlapping the shaft
        if ( boundingBoxSide != ShaftPlanePosition::OUTSIDE &&
             ( boundingBoxSide == ShaftPlanePosition::INSIDE || shaftPatchTest(patch) != ShaftPlanePosition::OUTSIDE ) ) {
            culledPatchList->add(patch);
        }
    }
    return culledPatchList;
}

/**
Adds the geometry to the candidateList, possibly duplicating if it
was created during previous shaft culling
*/
void
Shaft::keep(Geometry *geometry, java::ArrayList<Geometry *> *candidateList) {
    if ( geometry->omit ) {
        return;
    }

    if ( geometry->shaftCullGeometry && geometry->className == GeometryClassId::PATCH_SET ) {
        Geometry *newGeometry = geomDuplicateIfPatchSet(geometry);
        newGeometry->shaftCullGeometry = true;
        candidateList->add(newGeometry);
    } else {
        candidateList->add(geometry);
    }
}

/**
Breaks the geometry into it's components and does shaft culling on the components
*/
void
Shaft::shaftCullOpen(Geometry *geometry, java::ArrayList<Geometry *> *candidateList, ShaftCullStrategy strategy) {
    if ( geometry->omit ) {
        return;
    }

    if ( geometry->isCompound() ) {
        const Compound *compound = geometry->compoundData;
        doCulling(compound->children, candidateList, strategy);
    } else {
        const java::ArrayList<Patch *> *geometryPatchesList = geomPatchArrayListReference(geometry);
        java::ArrayList<Patch *> *culledPatches = cullPatches(geometryPatchesList);

        if ( culledPatches->size() > 0 ) {
            Geometry *newGeometry = geomCreatePatchSet(culledPatches);
            newGeometry->shaftCullGeometry = true;
            newGeometry->isDuplicate = false;
            candidateList->add(newGeometry);
        }
        delete culledPatches;
    }
}

/**
Tests the geometry with respect to the shaft: if the geometry is inside or overlaps
the shaft, it is copied to the shaft or broken open depending on
the current shaft culling strategy
*/
void
Shaft::cullGeometry(
    Geometry *geometry,
    java::ArrayList<Geometry *> *candidateList,
    const ShaftCullStrategy strategy)
{
    if ( geometry->className == GeometryClassId::PATCH_SET
        && (geometry->omit || patchIsOnOmitSet((Patch *)geometry)) ) {
        return;
    }

    // Unbounded geoms always overlap the shaft
    switch ( geometry->bounded ? boundingBoxTest(&geometry->boundingBox) : ShaftPlanePosition::OVERLAP ) {
        case ShaftPlanePosition::INSIDE:
            if ( strategy == ShaftCullStrategy::ALWAYS_OPEN && !closedGeometry(geometry) ) {
                shaftCullOpen(geometry, candidateList, strategy);
            } else {
                keep(geometry, candidateList);
            }
            break;
        case ShaftPlanePosition::OVERLAP:
            if ( closedGeometry(geometry) ) {
                keep(geometry, candidateList);
            } else {
                shaftCullOpen(geometry, candidateList, strategy);
            }
            break;
        default:
            break;
    }
}

/**
Adds all objects from world that overlap or lay inside the shaft to
candidateList

During shaft culling getPatchList geometries are created - they (and only they)
need to be destroyed when destroying a geometry candidate list created by
doCulling - for other kinds of geoms, only a pointer is copied
*/
void
Shaft::doCulling(
    const java::ArrayList<Geometry *> *world,
    java::ArrayList<Geometry *> *candidateList,
    ShaftCullStrategy strategy) {
    for ( int i = 0; world != nullptr && i < world->size() && !cut; i++ ) {
        cullGeometry(world->get(i), candidateList, strategy);
    }
}

/**
Frees the memory occupied by a candidate list produced by doCulling
*/
void
freeCandidateList(java::ArrayList<Geometry *> *candidateList) {
    // Only destroy geoms that were generated for shaft culling
    for ( int i = 0; candidateList != nullptr && i < candidateList->size(); i++ ) {
        Geometry *geometry = candidateList->get(i);
        if ( geometry->shaftCullGeometry ) {
            geomDestroy(geometry);
        }
    }

    delete candidateList;
}
