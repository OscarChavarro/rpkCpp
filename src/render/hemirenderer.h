/* 
 * hemirenderer.H : tool class to render data defined on the
 *   hemisphere. 
 */

#ifndef _HEMIRENDERER_H_
#define _HEMIRENDERER_H_

#include "common/color.h"
#include "common/linealAlgebra/vectorMacros.h"
#include "scene/spherical.h"

typedef void (*CHRAcquireCallback)(Vector3D *vec, COORDSYS *coordsys,
                                   double phi, double theta, void *data,
                                   RGB *color, double *distance);

class CHRVertexData {
public:
    Vector3D point;
    Vector3D normal;
    RGB rgb;
};

class CHemisphereRenderer {
public:
    // Constructor : nr. of steps in phi and theta
    CHemisphereRenderer();

    ~CHemisphereRenderer();

    // AcquireData fills in the data on the hemisphere.
    // The callback is called for every phi and theta value.

    void Initialize(int Nphi, int Ntheta, Vector3D center,
                    COORDSYS *coordsys,
                    CHRAcquireCallback cb, void *cbData);

    void EnableRendering(bool flag);

    // Render : renders the hemispherical data

    void Render();

protected:
    bool m_renderEnabled;
    int m_phiSteps, m_thetaSteps;
    double m_deltaPhi, m_deltaTheta;
    Vector3D m_center;
    COORDSYS m_coordsys;
    CHRVertexData *m_renderData; // Data in WC, together with normals
    CHRVertexData m_top;
};


#endif /* _HEMIRENDERER_H_ */
