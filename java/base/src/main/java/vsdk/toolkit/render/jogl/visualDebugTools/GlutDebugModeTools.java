package vsdk.toolkit.render.jogl.visualDebugTools;

public final class GlutDebugModeTools {
    private GlutDebugModeTools() {
    }

    public static GlutDebugMode nextMode(GlutDebugMode mode) {
        if ( mode == GlutDebugMode.GALERKIN_ELEMENT_HIERARCHY ) {
            return GlutDebugMode.RADIANCE_SCENE;
        }
        return GlutDebugMode.GALERKIN_ELEMENT_HIERARCHY;
    }

    public static String modeName(GlutDebugMode mode) {
        if ( mode == null ) {
            return "UNKNOWN";
        }
        switch ( mode ) {
            case RADIANCE_SCENE:
                return "RADIANCE_SCENE";
            case GALERKIN_ELEMENT_HIERARCHY:
                return "GALERKIN_ELEMENT_HIERARCHY";
            default:
                return "UNKNOWN";
        }
    }
}
