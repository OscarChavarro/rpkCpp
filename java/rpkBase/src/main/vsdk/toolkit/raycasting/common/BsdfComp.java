/**
Some bsdf component stuff.
*/

package vsdk.toolkit.raycasting.common;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.BsdfComponentFlag;

public class BsdfComp {
    private static final int BSDF_COMPONENTS = 6;
    private static final int BSDF_ALL_COMPONENTS = BsdfComponent.BRDF_DIFFUSE_COMPONENT
        | BsdfComponent.BRDF_GLOSSY_COMPONENT
        | BsdfComponent.BRDF_SPECULAR_COMPONENT
        | BsdfComponent.BTDF_DIFFUSE_COMPONENT
        | BsdfComponent.BTDF_GLOSSY_COMPONENT
        | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    public ColorRgbMutable[] comp;

    public BsdfComp() {
        comp = new ColorRgbMutable[BSDF_COMPONENTS];
        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            comp[i] = new ColorRgbMutable();
        }
    }

    public ColorRgbMutable get(int index) {
        return comp[index];
    }

    public ColorRgbMutable[] asArray() {
        return comp;
    }

    public void Clear() {
        Clear((byte)BSDF_ALL_COMPONENTS);
    }

    public void Clear(byte flags) {
        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            if ((flags & (BsdfComponentFlag.bsdfIndexToComp(i))) != 0) {
                comp[i] = new ColorRgbMutable();
            }
        }
    }

    public void Fill(ColorRgbMutable col) {
        Fill(col, (byte)BSDF_ALL_COMPONENTS);
    }

    public void Fill(ColorRgbMutable col, byte flags) {
        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            if ((flags & (BsdfComponentFlag.bsdfIndexToComp(i))) != 0) {
                comp[i] = new ColorRgbMutable(col.getR(), col.getG(), col.getB());
            }
        }
    }

    public ColorRgbMutable Sum() {
        return Sum((byte)BSDF_ALL_COMPONENTS);
    }

    public ColorRgbMutable Sum(byte flags) {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;

        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            if ((flags & (BsdfComponentFlag.bsdfIndexToComp(i))) != 0) {
                r += comp[i].getR();
                g += comp[i].getG();
                b += comp[i].getB();
            }
        }

        return new ColorRgbMutable(r, g, b);
    }
}
