#include "common/error.h"
#include "raycasting/raytracing/densitybuffer.h"
#include "raycasting/raytracing/densitykernel.h"

// Implementation CDensityHitList

const int DHL_ARRAYSIZE = 20;

CDensityHitList::CDensityHitList() {
    m_first = new CDensityHitArray(DHL_ARRAYSIZE);
    m_last = m_first;
    m_cacheCurrent = nullptr;

    m_numHits = 0;
}

CDensityHitList::~CDensityHitList() {
    CDensityHitArray *tmpDA;

    while ( m_first ) {
        tmpDA = m_first;
        m_first = m_first->m_next;
        delete tmpDA;
    }
}

CDensityHit CDensityHitList::operator[](int i) {
    if ( i >= m_numHits ) {
        Fatal(-1, __FILE__ ":CDensityHitList::operator[]", "Index 'i' out of bounds");
    }

    if ( !m_cacheCurrent || (i < m_cacheLowerLimit)) {
        m_cacheCurrent = m_first;
        m_cacheLowerLimit = 0;
    }

    // Wanted ponit is beyond m_cacheCurrent
    while ( i >= m_cacheLowerLimit + DHL_ARRAYSIZE ) {
        m_cacheCurrent = m_cacheCurrent->m_next;
        m_cacheLowerLimit += DHL_ARRAYSIZE;
    }

    // Wanted point is in current cache block

    return ((*m_cacheCurrent)[i - m_cacheLowerLimit]);
}

void CDensityHitList::Add(CDensityHit &hit) {
    if ( !m_last->Add(hit)) {
        // New array needed

        m_last->m_next = new CDensityHitArray(DHL_ARRAYSIZE);
        m_last = m_last->m_next;

        m_last->Add(hit); // Supposed not to fail
    }

    m_numHits++;
}



// CDensityBuffer implementation

CDensityBuffer::CDensityBuffer(ScreenBuffer *screen, BP_BASECONFIG *bcfg) {
    m_screen = screen;
    m_bcfg = bcfg;

    m_xmin = m_screen->GetScreenXMin();
    m_xmax = m_screen->GetScreenXMax();
    m_ymin = m_screen->GetScreenYMin();
    m_ymax = m_screen->GetScreenYMax();

    printf("Density Buffer :\nXmin %f, Ymin %f, Xmax %f, Ymax %f\n",
           m_xmin, m_ymin, m_xmax, m_ymax);

}

CDensityBuffer::~CDensityBuffer() {
    /*
      int i,j;


      for(i= 0; i < DHA_XRES; i++)
      {
        for(j = 0; j < DHA_YRES; j++)
      {
      printf("X %i, Y %i, Samples %i\n", i,j, m_hitGrid[i][j].StoredHits());
      }
    }
    */
}

void CDensityBuffer::Add(float x, float y, COLOR col, float pdf, float w) {
    float factor = m_screen->GetPixXSize() * m_screen->GetPixYSize()
                   * (float) m_bcfg->totalSamples;
    COLOR tmpCol;

    // printf("Hit %f %f Index %i %i\n",hit.m_x, hit.m_y, XIndex(x), XIndex(y));

    if ( COLORAVERAGE(col) > EPSILON ) {
        COLORSCALE(factor, col, tmpCol); // Undo part of flux to rad factor

        CDensityHit hit(x, y, tmpCol, pdf, w);

        m_hitGrid[XIndex(x)][YIndex(y)].Add(hit);
    }
}


ScreenBuffer *CDensityBuffer::Reconstruct() {
    // For all samples -> compute pixel coverage

    // Kernel size. Now spread over 3 pixels
    float h = 8 * MAX(m_screen->GetPixXSize(), m_screen->GetPixYSize())
              / sqrt((double) m_bcfg->samplesPerPixel);

    printf("h = %f\n", h);

    m_screen->ScaleRadiance(0.0); // Hack !!

    int i, j, k, maxk;

    CDensityHit hit;
    CKernel2D kernel;
    Vector2D center;

    kernel.SetH(h);


    for ( i = 0; i < DHA_XRES; i++ ) {
        for ( j = 0; j < DHA_YRES; j++ ) {
            // printf("X %i, Y %i, Samples %i\n", i,j, m_hitGrid[i][j].StoredHits());

            maxk = m_hitGrid[i][j].StoredHits();

            for ( k = 0; k < maxk; k++ ) {
                hit = (m_hitGrid[i][j])[k];

                center.u = hit.m_x;
                center.v = hit.m_y;

                kernel.Cover(center, 1.0 / m_bcfg->totalSamples, hit.m_color, m_screen);
            }
        }
    }

    return m_screen;
}


ScreenBuffer *CDensityBuffer::ReconstructVariable(ScreenBuffer *dest,
                                                  float baseSize) {
    // For all samples -> compute pixel coverage

    // Base Kernel size. Now spread over a number of pixels

    dest->ScaleRadiance(0.0); // Hack !!

    int i, j, k, maxk;

    CDensityHit hit;
    CKernel2D kernel;
    Vector2D center;

    for ( i = 0; i < DHA_XRES; i++ ) {
        for ( j = 0; j < DHA_YRES; j++ ) {
            // printf("X %i, Y %i, Samples %i\n", i,j, m_hitGrid[i][j].StoredHits());

            maxk = m_hitGrid[i][j].StoredHits();

            for ( k = 0; k < maxk; k++ ) {
                hit = (m_hitGrid[i][j])[k];

                center.u = hit.m_x;
                center.v = hit.m_y;

                kernel.VarCover(center, hit.m_color, m_screen, dest, m_bcfg->totalSamples,
                                m_bcfg->samplesPerPixel, baseSize);
            }
        }
    }

    return dest;
}

/*
ScreenBuffer *CDensityBuffer::ReconstructVariable(ScreenBuffer *dest)
{
  // For all samples -> compute pixel coverage

  // Base Kernel size. Now spread over a number of pixels
  float screenScale = MAX(m_screen->GetPixXSize(), m_screen->GetPixYSize());
  float screenFactor = m_screen->GetPixXSize() * m_screen->GetPixYSize();
  float B = 8 * screenScale;

  // Use optimal N dependency for fixed kernels
  float Bn = B * (pow((double)m_bcfg->samplesPerPixel, (-1.0/5.0)));

  float h;

  dest->ScaleRadiance(0.0); // Hack !!

  int i,j,k, maxk;

  CDensityHit hit;
  CKernel2D kernel;
  Vector2D center;

  

  for(i= 0; i < DHA_XRES; i++)
  {
    for(j = 0; j < DHA_YRES; j++)
    {
      // printf("X %i, Y %i, Samples %i\n", i,j, m_hitGrid[i][j].StoredHits());

      maxk = m_hitGrid[i][j].StoredHits();

      for(k = 0; k < maxk; k++)
      {
	hit = (m_hitGrid[i][j])[k];
	
	center.u = hit.m_x;
	center.v = hit.m_y;

	// Now compute h for this sample

	// Reference estimated function
	COLOR fe = m_screen->GetBiLinear(center.u, center.v);
	float avgFe = COLORAVERAGE(fe);
	float avgG = COLORAVERAGE(hit.m_color);

	if(avgFe > EPSILON)
	{
	  h = Bn * sqrt(avgG / avgFe);
	  // printf("fe %f G %f, h = %f\n", avgFe, avgG, h/screenScale);
	}
	else
	{
	  const float maxRatio = 10; // ???
	  h = Bn * maxRatio * screenScale;

	  printf("MaxRatio... h = %f\n", h/screenScale);
	}

	h = MAX(1.0*screenScale, h);

	kernel.SetH(h);
	kernel.Cover(center, 1.0/m_bcfg->totalSamples, hit.m_color, dest);
      }
    }
  }

  return dest;
}
*/
