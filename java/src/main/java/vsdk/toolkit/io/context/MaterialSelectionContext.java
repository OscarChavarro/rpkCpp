package vsdk.toolkit.io.context;

import java.util.ArrayList;
import vsdk.toolkit.material.Material;

public class MaterialSelectionContext {
    public Material currentMaterial;
    public String currentMaterialName;
    public ArrayList<Material> materials;

    public MaterialSelectionContext() {
        currentMaterial = null;
        currentMaterialName = null;
        materials = null;
    }
}
