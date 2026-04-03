package vsdk.toolkit.material;

public class RefractionIndex {
    private float nr;
    private float ni;

    public float complexToGeometricRefractionIndex() {
        float f1 = (nr - 1.0f);
        f1 = f1 * f1 + ni * ni;

        float f2 = (nr + 1.0f);
        f2 = f2 * f2 + ni * ni;

        float sqrtF = (float)Math.sqrt(f1 / f2);

        return (1.0f + sqrtF) / (1.0f - sqrtF);
    }

    public float getNr() {
        return nr;
    }

    public float getNi() {
        return ni;
    }

    public void set(float inNr, float inNi) {
        nr = inNr;
        ni = inNi;
    }
}
