package vsdk.toolkit.material;

/**
Compute an approximate geometric IOR from a complex IOR (cfr. Gr.Gems II, p289)
*/

public class RefractionIndex {
    private double nr;
    private double ni;

    public double complexToGeometricRefractionIndex() {
        double f1 = (nr - 1.0);
        f1 = f1 * f1 + ni * ni;

        double f2 = (nr + 1.0);
        f2 = f2 * f2 + ni * ni;

        double sqrtF = (double)Math.sqrt(f1 / f2);

        return (1.0 + sqrtF) / (1.0 - sqrtF);
    }

    public double getNr() {
        return nr;
    }

    public double getNi() {
        return ni;
    }

    public void set(double inNr, double inNi) {
        nr = inNr;
        ni = inNi;
    }
}
