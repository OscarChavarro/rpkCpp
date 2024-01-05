/* defaults.h: rad program defaults */

#ifndef _RPK_DEFAULTS_H_
#define _RPK_DEFAULTS_H_

#ifndef WEBBROWSER
    #define WEBBROWSER "netscape %s &"
#endif
#ifndef RPKHOME
    #define RPKHOME "http://www.cs.kuleuven.ac.be/cwis/research/graphics/RENDERPARK/"
#endif
#ifndef RPKHTMLDOC
    #define RPKHTMLDOC "http://www.cs.kuleuven.ac.be/cwis/research/graphics/RENDERPARK/DOC/"
#endif

/* default radiance method: a short name of a radiance method or "none" */
#define DEFAULT_RADIANCE_METHOD "none"

/* raytracing defaults */
#define DEFAULT_RAYTRACING_METHOD "stochastic"

/* default rendering options */
#define DEFAULT_DISPLAY_LISTS false
#define DEFAULT_SMOOTH_SHADING true
#define DEFAULT_BACKFACE_CULLING true
#define DEFAULT_OUTLINE_DRAWING false
#define DEFAULT_BOUNDING_BOX_DRAWING false
#define DEFAULT_CLUSTER_DRAWING false

#define DEFAULT_OUTLINE_COLOR {0.5, 0.0, 0.0}
#define DEFAULT_BOUNDING_BOX_COLOR {0.5, 0.0, 1.0}
#define DEFAULT_CLUSTER_COLOR {1.0, 0.5, 0.0}

/* default virtual camera */
#define DEFAULT_EYEP {10.0, 0.0, 0.0}
#define DEFAULT_LOOKP { 0.0, 0.0, 0.0}
#define DEFAULT_UPDIR { 0.0, 0.0, 1.0}
#define DEFAULT_FOV 22.5
#define DEFAULT_BACKGROUND_COLOR {0.0, 0.0, 0.0}

/* mgf defaults */
#define DEFAULT_NQCDIVS 4
#define DEFAULT_FORCE_ONESIDEDNESS true
#define DEFAULT_MONOCHROME false

/* Tone mapping defaults */
#define DEFAULT_TM_LWA 10.0
#define DEFAULT_TM_LDMAX 100.0
#define DEFAULT_TM_CMAX 50.0

#endif
