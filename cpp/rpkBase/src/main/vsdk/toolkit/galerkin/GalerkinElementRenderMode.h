#ifndef __GALERKIN_ELEMENT_RENDER_MODE__
#define __GALERKIN_ELEMENT_RENDER_MODE__

// Element render modes, additive
enum GalerkinElementRenderMode {
    OUTLINE = 0x01,
    FLAT = 0x02,
    GOURAUD = 0x04,
    NOT_A_REGULAR_SUB_ELEMENT = 0x08
};

#endif
