package vsdk.toolkit.io.context;

import java.util.ArrayList;
import vsdk.toolkit.material.RpkMaterial;

public class MaterialSelectionContext {
    public RpkMaterial currentMaterial;
    public String currentMaterialName;
    public ArrayList<RpkMaterial> materials;

    public MaterialSelectionContext() {
        currentMaterial = null;
        currentMaterialName = null;
        materials = null;
    }
}
