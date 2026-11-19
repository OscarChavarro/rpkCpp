package vsdk.toolkit.material;

public final class BsdfComponentFlag {
    private BsdfComponentFlag() {
    }

    public static int bsdfIndexToComp(int index) {
        return 1 << index;
    }

    public static int getBrdfFlags(int bsflags) {
        return bsflags & XxdfComponentFlagInfo.ALL_COMPONENTS;
    }

    public static int getBtdfFlags(int bsflags) {
        return (bsflags >> XxdfComponentFlagInfo.XXDF_COMPONENTS) & XxdfComponentFlagInfo.ALL_COMPONENTS;
    }
}
