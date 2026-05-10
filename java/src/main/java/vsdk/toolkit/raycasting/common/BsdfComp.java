/**
Some bsdf component stuff.
*/

package vsdk.toolkit.raycasting.common;

import vsdk.toolkit.common.color.ColorRgb;
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

    public ColorRgb[] comp;

    public BsdfComp() {
        comp = new ColorRgb[BSDF_COMPONENTS];
        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            comp[i] = new ColorRgb();
        }
    }

    public ColorRgb get(int index) {
        return comp[index];
    }

    public ColorRgb[] asArray() {
        return comp;
    }

    public void Clear() {
        Clear((byte)BSDF_ALL_COMPONENTS);
    }

    public void Clear(byte flags) {
        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            if ((flags & (BsdfComponentFlag.bsdfIndexToComp(i))) != 0) {
                comp[i].clear();
            }
        }
    }

    public void Fill(ColorRgb col) {
        Fill(col, (byte)BSDF_ALL_COMPONENTS);
    }

    public void Fill(ColorRgb col, byte flags) {
        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            if ((flags & (BsdfComponentFlag.bsdfIndexToComp(i))) != 0) {
                comp[i].set(col.r, col.g, col.b);
            }
        }
    }

    public ColorRgb Sum() {
        return Sum((byte)BSDF_ALL_COMPONENTS);
    }

    public ColorRgb Sum(byte flags) {
        ColorRgb result = new ColorRgb();

        result.clear();

        for (int i = 0; i < BSDF_COMPONENTS; i++) {
            if ((flags & (BsdfComponentFlag.bsdfIndexToComp(i))) != 0) {
                result.add(result, comp[i]);
            }
        }

        return result;
    }
}
