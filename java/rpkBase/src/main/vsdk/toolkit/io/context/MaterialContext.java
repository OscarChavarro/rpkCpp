package vsdk.toolkit.io.context;

public class MaterialContext {
    public int clock; // Incremented each change -- resettable
    public boolean sided; // true if surface is 1-sided, false for 2-sided
    public double nr; // Index of refraction, real and imaginary
    public double ni;
    public double rd; // Diffuse reflectance
    public ColorContext rd_c; // Diffuse reflectance color
    public double td; // Diffuse transmittance
    public ColorContext td_c; // Diffuse transmittance color
    public double ed; // Diffuse emittance
    public ColorContext ed_c; // Diffuse emittance color
    public double rs; // Specular reflectance
    public ColorContext rs_c; // Specular reflectance color
    public double rs_a; // Specular reflectance roughness
    public double ts; // Specular transmittance
    public ColorContext ts_c; // Specular transmittance color
    public double ts_a; // Specular transmittance roughness

    public MaterialContext() {
        clock = 0;
        sided = false;
        nr = 0.0;
        ni = 0.0;
        rd = 0.0;
        rd_c = new ColorContext();
        td = 0.0;
        td_c = new ColorContext();
        ed = 0.0;
        ed_c = new ColorContext();
        rs = 0.0;
        rs_c = new ColorContext();
        rs_a = 0.0;
        ts = 0.0;
        ts_c = new ColorContext();
        ts_a = 0.0;
    }

    public void copy(MaterialContext source) {
        if (source == null) {
            return;
        }
        clock = source.clock;
        sided = source.sided;
        nr = source.nr;
        ni = source.ni;
        rd = source.rd;
        rd_c.copy(source.rd_c);
        td = source.td;
        td_c.copy(source.td_c);
        ed = source.ed;
        ed_c.copy(source.ed_c);
        rs = source.rs;
        rs_c.copy(source.rs_c);
        rs_a = source.rs_a;
        ts = source.ts;
        ts_c.copy(source.ts_c);
        ts_a = source.ts_a;
    }
}
