/**
Tool class to render data defined on the hemisphere
*/
#include "render/hemirenderer.h"
#include "common/linealAlgebra/vectorMacros.h"
#include "material/color.h"
#include "shared/renderhook.h"
#include "render/opengl.h"

/**
Constructor : nr. of steps in phi and theta
*/
CHemisphereRenderer::CHemisphereRenderer(): m_deltaPhi(), m_deltaTheta() {
    m_phiSteps = 0;
    m_thetaSteps = 0;
    m_renderData = nullptr;
    m_renderEnabled = false;
}

CHemisphereRenderer::~CHemisphereRenderer() {
    if ( m_renderData != nullptr ) {
        EnableRendering(false);
        delete[] m_renderData;
        m_renderData = nullptr;
    }
}

// AcquireData fills in the data on the hemisphere.
// The callback is called for every phi and theta value.
void
CHemisphereRenderer::Initialize(
    int Nphi,
    int Ntheta,
    Vector3D center,
    COORDSYS *coordsys,
    CHRAcquireCallback cb,
    void *cbData)
{
    Vector3D vec;
    RGB col{};
    double curPhi, curTheta, dist;
    int t, p, index;

    // Copy parameters

    m_center = center;
    m_coordsys = *coordsys;

    // Allocate enough room for the data

    if ((Nphi != m_phiSteps) || (Ntheta != m_thetaSteps)) {
        // different resolution
        if ( m_renderData != nullptr ) {
            delete[] m_renderData;
            m_renderData = nullptr;
        }

        m_phiSteps = Nphi;
        m_thetaSteps = Ntheta;
        m_deltaPhi = 2.0 * M_PI / m_phiSteps;
        m_deltaTheta = 0.5 * M_PI / m_thetaSteps;
    }

    if ( m_renderData == nullptr ) {
        m_renderData = new CHRVertexData[m_phiSteps * (m_thetaSteps - 1)];
    }


    // Now fill the data array

    curTheta = M_PI / 2.0;  // theta = 0 -> Z-axis = top of hemisphere
    index = 0;

    for ( t = 0; t < m_thetaSteps - 1; t++ ) {
        curPhi = 0.0;

        for ( p = 0; p < m_phiSteps; p++ ) {
            // Calculate WC vector

            sphericalCoordToVector(&m_coordsys, &curPhi, &curTheta, &vec);

            cb(&vec, &m_coordsys, curPhi, curTheta, cbData, &col, &dist);

            VECTORSUMSCALED(m_center, dist, vec, m_renderData[index].point);
            VECTORCOPY(vec, m_renderData[index].normal);
            m_renderData[index].rgb = col;

            curPhi += m_deltaPhi;
            index++;
        }

        curTheta -= m_deltaTheta;
    }

    // Now fill in the top of the hemisphere

    curPhi = 0;
    curTheta = 0;

    sphericalCoordToVector(&m_coordsys, &curPhi, &curTheta, &vec);

    cb(&vec, &m_coordsys, curPhi, curTheta, cbData, &col, &dist);

    VECTORSUMSCALED(m_center, dist, vec, m_top.point);
    VECTORCOPY(vec, m_top.normal);
    m_top.rgb = col;
}

// Render : renders the hemispherical data

void CHemisphereRenderer::Render() {
    int t, p, index1, index2;

    if ( m_renderData == nullptr ) {
        return;
    }

    index1 = 0; // First row
    index2 = m_phiSteps; // Second row

    for ( t = 0; t < m_thetaSteps - 2; t++ ) {
        // Start a new triangle strip
        openGlRenderBeginTriangleStrip();

        for ( p = 0; p < m_phiSteps; p++ ) {
            openGlRenderNextTrianglePoint(&m_renderData[index2].point,
                                          &m_renderData[index2].rgb);
            openGlRenderNextTrianglePoint(&m_renderData[index1].point,
                                          &m_renderData[index1].rgb);
            index1++;
            index2++;
        }

        // Send first positions again to close strip

        openGlRenderNextTrianglePoint(&m_renderData[index1].point,
                                      &m_renderData[index1].rgb);
        openGlRenderNextTrianglePoint(&m_renderData[index1 - m_phiSteps].point,
                                      &m_renderData[index1 - m_phiSteps].rgb);

        // End triangle strip

        openGlRenderEndTriangleStrip();
    }

    // Now render the top of the hemisphere

    // Start a new triangle strip
    openGlRenderBeginTriangleStrip();

    for ( p = 0; p < m_phiSteps; p++ ) {
        openGlRenderNextTrianglePoint(&m_top.point,
                                      &m_top.rgb);
        openGlRenderNextTrianglePoint(&m_renderData[index1].point,
                                      &m_renderData[index1].rgb);
        index1++;
    }

    // Send first positions again to close strip

    openGlRenderNextTrianglePoint(&m_top.point,
                                  &m_top.rgb);
    openGlRenderNextTrianglePoint(&m_renderData[index1 - m_phiSteps].point,
                                  &m_renderData[index1 - m_phiSteps].rgb);

    // End triangle strip

    openGlRenderEndTriangleStrip();
}

// Callback needed for the render hook

static void RenderHemisphereHook(void *data) {
    CHemisphereRenderer *hr = (CHemisphereRenderer *) data;

    hr->Render();
}


void CHemisphereRenderer::EnableRendering(bool flag) {
    if ( flag ) {
        if ( !m_renderEnabled ) {
            addRenderHook(RenderHemisphereHook, this);
        }
    } else {
        if ( m_renderEnabled ) {
            removeRenderHook(RenderHemisphereHook, this);
        }
    }

    m_renderEnabled = flag;
}

