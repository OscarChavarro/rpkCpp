package vsdk.toolkit.scene;

import java.io.OutputStream;
import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public abstract class RadianceMethod {
    public RadianceMethodAlgorithm className;

    public RadianceMethod() {
    }

    public abstract String getRadianceMethodName();

    // A function to parse command line arguments for the method
    public abstract void parseOptions(int[] argc, String[] argv);

    // Initializes the current scene for radiance computations. Called when a new
    // scene is loaded or when selecting a particular radiance algorithm
    public abstract void initialize(Scene scene, ToneMappingContext toneMapOptions);

    // Does one step or iteration of the radiance computation, typically a unit
    // of computations after which the scene is to be redrawn. Returns TRUE when
    // done
    public abstract boolean doStep(Scene scene, RenderOptions renderOptions);

    // Terminates radiance computations on the current scene
    public abstract void terminate(ArrayList<Patch> scenePatches);

    // Returns the radiance being emitted from the specified patch, at
    // the point with given (u,v) parameters and into the given direction
    public abstract ColorRgb getRadiance(
        Camera camera,
        Patch patch,
        double u,
        double v,
        Vector3D dir,
        RenderOptions renderOptions);

    // Allocates memory for the radiance data for the given patch. Fills in the pointer in patch->radianceData
    public abstract Element createPatchData(Patch patch);

    // Destroys the radiance data for the patch. Clears the patch->radianceData pointer
    public abstract void destroyPatchData(Patch patch);

    // Returns a string with statistics information about the current run so far
    public abstract String getStats();

    // If defined, this routine will save the current model in VRML format.
    // If not defined, the default method implemented in write vrml.[ch] will
    // be used
    public abstract void writeVRML(
        Camera camera,
        OutputStream outputStream,
        RenderOptions renderOptions);
}
