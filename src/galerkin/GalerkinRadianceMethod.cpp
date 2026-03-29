/**
Galerkin radiosity, with the following variants:
- With or without hierarchical refinement
- With or without clusters
- With Jacobi, Gauss-Seidel or South well iterations
- With potential-driven or not
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/Error.h"
#include "common/Statistics.h"
#include "tonemap/ToneMap.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/wrl/VrmlWriter.h"
#include "render/GlutDebugTools.h"
#include "render/Opengl.h"
#include "galerkin/GalerkinBasis.h"
#include "galerkin/GalerkinRadianceMethod.h"
#include "galerkin/processing/ClusterCreationStrategy.h"
#include "galerkin/processing/GatheringClusteredStrategy.h"
#include "galerkin/processing/GatheringSimpleStrategy.h"
#include "galerkin/processing/ScratchVisibilityStrategy.h"
#include "galerkin/processing/ShootingStrategy.h"

static constexpr int STRING_LENGTH = 2000;

GalerkinState GalerkinRadianceMethod::galerkinState;
java::io::OutputStream *GalerkinRadianceMethod::vrmlOutputStream = nullptr;
int GalerkinRadianceMethod::numberOfWrites = 0;
int GalerkinRadianceMethod::vertexId = 0;

java::lang::String
GalerkinRadianceMethod::formatToString(const char *format, va_list arguments) {
    if ( format == nullptr ) {
        return java::lang::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::util::Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return java::lang::String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::lang::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    java::util::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    java::lang::String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

void
GalerkinRadianceMethod::writeFormatted(const char *format, ...) {
    if ( vrmlOutputStream == nullptr || format == nullptr ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    java::lang::String text = GalerkinRadianceMethod::formatToString(format, arguments);
    va_end(arguments);

    if ( text.isEmpty() ) {
        return;
    }

    vsdk::PersistenceElement::writeBytes(
        *vrmlOutputStream,
        reinterpret_cast<const unsigned char *>(text.toCString()),
        text.length());
}

void
GalerkinRadianceMethod::appendStatsText(char *buffer, int *offset, const char *format, ...) {
    if ( *offset >= STRING_LENGTH - 1 ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    java::lang::String text = GalerkinRadianceMethod::formatToString(format, arguments);
    va_end(arguments);

    const int written = text.length();
    if ( written <= 0 ) {
        return;
    }

    const int available = STRING_LENGTH - *offset - 1;
    const int copied = (written > available) ? available : written;
    if ( copied > 0 ) {
        std::memcpy(&buffer[*offset], text.toCString(), static_cast<std::size_t>(copied));
        *offset += copied;
        buffer[*offset] = '\0';
    }
}

void
GalerkinRadianceMethod::writeVertexCoord(const Vector3D *p) {
    if ( numberOfWrites > 0 ) {
        writeFormatted("%s", ", ");
    }
    numberOfWrites++;
    if ( numberOfWrites % 4 == 0 ) {
        writeFormatted("%s", "\n\t  ");
    }
    writeFormatted("%g %g %g", p->x, p->y, p->z);
    vertexId++;
}

void
GalerkinRadianceMethod::writeVertexCoords(Element *element) {
    const GalerkinElement *galerkinElement = static_cast<GalerkinElement *>(element);
    Vector3D v[8];
    int numberOfVertices = galerkinElement->vertices(v);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        writeVertexCoord(&v[i]);
    }
}

void
GalerkinRadianceMethod::writeCoords() {
    numberOfWrites = vertexId = 0;
    writeFormatted("%s", "\tcoord Coordinate {\n\t  point [ ");
    GalerkinRadianceMethod::galerkinState.topCluster->traverseAllLeafElements(writeVertexCoords);
    writeFormatted("%s", " ] ");
    writeFormatted("%s", "\n\t}\n");
}

void
GalerkinRadianceMethod::writeVertexColor(const ColorRgb *color) {
    if ( numberOfWrites > 0 ) {
        writeFormatted("%s", ", ");
    }
    numberOfWrites++;
    if ( numberOfWrites % 4 == 0 ) {
        writeFormatted("%s", "\n\t  ");
    }
    writeFormatted("%.3g %.3g %.3g", color->r, color->g, color->b);
    vertexId++;
}

void
GalerkinRadianceMethod::writeVertexColors(Element *element) {
    const GalerkinElement *galerkinElement = static_cast<GalerkinElement *>(element);
    ColorRgb vertexRadiosity[4];
    int i;

    if ( galerkinElement->patch->numberOfVertices == 3 ) {
        vertexRadiosity[0] = GalerkinBasis::radianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0);
        vertexRadiosity[1] = GalerkinBasis::radianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0);
        vertexRadiosity[2] = GalerkinBasis::radianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0);
    } else {
        vertexRadiosity[0] = GalerkinBasis::radianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0);
        vertexRadiosity[1] = GalerkinBasis::radianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0);
        vertexRadiosity[2] = GalerkinBasis::radianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 1.0);
        vertexRadiosity[3] = GalerkinBasis::radianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0);
    }

    if ( GalerkinRadianceMethod::galerkinState.useAmbientRadiance ) {
        ColorRgb reflectivity = galerkinElement->patch->radianceData->Rd;
        ColorRgb ambient;

        ambient.scalarProduct(reflectivity, GalerkinRadianceMethod::galerkinState.ambientRadiance);
        for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
            vertexRadiosity[i].add(vertexRadiosity[i], ambient);
        }
    }

    for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
        ColorRgb col{};
        ToneMap::radianceToRgb(vertexRadiosity[i], &col);
        writeVertexColor(&col);
    }
}

void
GalerkinRadianceMethod::writeVertexColorsTopCluster() {
    vertexId = numberOfWrites = 0;
    writeFormatted("%s", "\tcolor Color {\n\t  color [ ");
    GalerkinRadianceMethod::galerkinState.topCluster->traverseAllLeafElements(writeVertexColors);
    writeFormatted("%s", " ] ");
    writeFormatted("%s", "\n\t}\n");
}

void
GalerkinRadianceMethod::writeColors(const RenderOptions *renderOptions) {
    if ( !renderOptions->smoothShading ) {
        Error::warning(nullptr, "I assume you want a smooth shaded model ...");
    }
    writeFormatted("\tcolorPerVertex %s\n", "TRUE");
    writeVertexColorsTopCluster();
}

void
GalerkinRadianceMethod::writeCoordIndex(int index) {
    numberOfWrites++;
    if ( numberOfWrites % 20 == 0 ) {
        writeFormatted("%s", "\n\t  ");
    }
    writeFormatted("%d ", index);
}

void
GalerkinRadianceMethod::writeCoordIndices(Element *element) {
    const GalerkinElement *galerkinElement = static_cast<GalerkinElement *>(element);
    for ( int i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
        writeCoordIndex(vertexId);
        vertexId++;
    }
    writeCoordIndex(-1);
}

void
GalerkinRadianceMethod::writeCoordIndicesTopCluster() {
    vertexId = numberOfWrites = 0;
    writeFormatted("%s", "\tcoordIndex [ ");
    GalerkinRadianceMethod::galerkinState.topCluster->traverseAllLeafElements(writeCoordIndices);
    writeFormatted("%s", " ]\n");
}

void
GalerkinRadianceMethod::freeMemory() {
    if ( GalerkinRadianceMethod::galerkinState.scratch != nullptr ) {
        delete GalerkinRadianceMethod::galerkinState.scratch;
        GalerkinRadianceMethod::galerkinState.scratch = nullptr;
    }
}

GalerkinRadianceMethod::GalerkinRadianceMethod() {
    className = GALERKIN;
    gatheringStrategy = nullptr;
}

GalerkinRadianceMethod::~GalerkinRadianceMethod() {
    if ( galerkinState.topCluster != nullptr ) {
        galerkinState.topCluster = nullptr;
    }
    if ( gatheringStrategy != nullptr ) {
        delete gatheringStrategy;
        gatheringStrategy = nullptr;
    }
}

void
GalerkinRadianceMethod::renderElementHierarchy(const GalerkinElement *element, const RenderOptions *renderOptions) {
    if ( element->regularSubElements == nullptr ) {
        element->render(renderOptions);
    } else {
        for ( int i = 0; i < 4; i++ ) {
            renderElementHierarchy(static_cast<GalerkinElement *>(element->regularSubElements[i]), renderOptions);
        }
    }
}

void
GalerkinRadianceMethod::galerkinRenderPatch(const Patch *patch, const Camera * /*camera*/, const RenderOptions *renderOptions) {
    renderElementHierarchy(GalerkinElement::fromPatch(patch), renderOptions);
}

/**
For counting how much CPU time was used for the computations
*/
void
GalerkinRadianceMethod::updateCpuSecs() {
    const long long t = java::lang::System::nanoTime();
    galerkinState.cpuSeconds += static_cast<float>(static_cast<double>(t - galerkinState.lastClock) / 1000000000.0);
    galerkinState.lastClock = t;
}

void
GalerkinRadianceMethod::patchInit(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb selfEmittanceRadiance = patch->radianceData->Ed;

    if ( galerkinState.useConstantRadiance ) {
        // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
        // '95, Dublin, Ireland, June 1995, p 336-344
        galerkinGetRadiance(patch).scalarProduct(reflectivity, galerkinState.constantRadiance);
        galerkinGetRadiance(patch).add(galerkinGetRadiance(patch), selfEmittanceRadiance);
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
            patch->radianceData->unShotRadiance[0].subtract(galerkinGetRadiance(patch), galerkinState.constantRadiance);
        }
    } else {
        galerkinSetRadiance(patch, selfEmittanceRadiance);
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
            patch->radianceData->unShotRadiance[0] = galerkinGetRadiance(patch);
        }
    }

    if ( galerkinState.importanceDriven ) {
        switch ( galerkinState.galerkinIterationMethod ) {
            case GalerkinIterationMethod::GAUSS_SEIDEL:
            case GalerkinIterationMethod::JACOBI:
                galerkinSetPotential(patch, patch->directPotential);
                break;
            case GalerkinIterationMethod::SOUTH_WELL:
                galerkinSetPotential(patch, patch->directPotential);
                galerkinSetUnShotPotential(patch, patch->directPotential);
                break;
            default:
                Error::fatal(-1, "patchInit", "Invalid iteration method");
        }
    }

    recomputePatchColor(patch);
}

/**
Recomputes the color of a patch using ambient radiance term, ... if requested for
*/
void
GalerkinRadianceMethod::recomputePatchColor(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb radVis;

    // Compute the patches color based on its radiance + ambient radiance if desired
    if ( galerkinState.useAmbientRadiance ) {
        radVis.scalarProduct(reflectivity, galerkinState.ambientRadiance);
        radVis.add(radVis, galerkinGetRadiance(patch));
        ToneMap::radianceToRgb(radVis, &patch->color);
    } else {
        ToneMap::radianceToRgb(galerkinGetRadiance(patch), &patch->color);
    }
    patch->computeVertexColors();
}

const char *
GalerkinRadianceMethod::getRadianceMethodName() const  {
    return "Galerkin";
}

void
GalerkinRadianceMethod::setStrategy() {
    if ( galerkinState.clustered ) {
        gatheringStrategy = new GatheringClusteredStrategy();
    } else {
        gatheringStrategy = new GatheringSimpleStrategy();
    }
}

void
GalerkinRadianceMethod::parseOptions(int * /*argc*/, char ** /*argv*/) {
}

void
GalerkinRadianceMethod::initialize(Scene *scene) {
    galerkinState.iterationNumber = 0;
    galerkinState.cpuSeconds = 0.0;

    GalerkinElement::initializeBasis();

    galerkinState.constantRadiance = GLOBAL_statistics.estimatedAverageRadiance;
    if ( galerkinState.useConstantRadiance ) {
        galerkinState.ambientRadiance.clear();
    } else {
        galerkinState.ambientRadiance = GLOBAL_statistics.estimatedAverageRadiance;
    }

    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        patchInit(scene->patchList->get(i));
    }

    galerkinState.topCluster = ClusterCreationStrategy::createClusterHierarchy(
        scene->clusteredRootGeometry, &galerkinState);

    // Create a scratch software renderer for various operations on clusters
    if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy::Z_VISIBILITY ) {
        ScratchVisibilityStrategy::scratchInit(&galerkinState);
    }

    // Global variables for scratch rendering
    galerkinState.lastClusterId = -1;
    galerkinState.lastEye.set(Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE);
}

bool
GalerkinRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    if ( galerkinState.iterationNumber < 0 ) {
        Error::error("doGalerkinOneStep", "method not initialized");
        return true; // Done, don't continue!
    }

    galerkinState.iterationNumber++;
    galerkinState.lastClock = java::lang::System::nanoTime();

    // And now the real work
    int done;

    switch ( galerkinState.galerkinIterationMethod ) {
        case GalerkinIterationMethod::JACOBI:
        case GalerkinIterationMethod::GAUSS_SEIDEL:
            done = gatheringStrategy->doGatheringIteration(scene, &galerkinState, renderOptions);
            break;
        case GalerkinIterationMethod::SOUTH_WELL:
            done = ShootingStrategy::doShootingStep(scene, &galerkinState, renderOptions);
            break;
        default:
            Error::fatal(2, "doGalerkinOneStep", "Invalid iteration method %d\n", galerkinState.galerkinIterationMethod);
    }

    updateCpuSecs();

    return done;
}

/**
Disposes of the cluster hierarchy
*/
void
GalerkinRadianceMethod::galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement) {
    if ( !clusterElement || !clusterElement->isCluster() ) {
        return;
    }

    for ( int i = 0; clusterElement->irregularSubElements != nullptr && i < clusterElement->irregularSubElements->size(); i++ ) {
        GalerkinRadianceMethod::galerkinDestroyClusterHierarchy(static_cast<GalerkinElement *>(clusterElement->irregularSubElements->get(i)));
    }
    delete clusterElement;
}

void
GalerkinRadianceMethod::terminate(java::ArrayList<Patch *> */*scenePatches*/) {
    if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy::Z_VISIBILITY ) {
        ScratchVisibilityStrategy::scratchTerminate(&galerkinState);
    }
    if ( galerkinState.topCluster != nullptr ) {
        GalerkinRadianceMethod::galerkinDestroyClusterHierarchy(galerkinState.topCluster);
        delete galerkinState.topCluster;
        galerkinState.topCluster = nullptr;
    }
}

ColorRgb
GalerkinRadianceMethod::getRadiance(
    Camera */*camera*/,
    Patch *patch,
    double u,
    double v,
    Vector3D /*dir*/,
    const RenderOptions */*renderOptions*/) const
{
    const GalerkinElement *leaf;
    ColorRgb rad;

    if ( patch->jacobian ) {
        patch->biLinearToUniform(&u, &v);
    }

    GalerkinElement *topLevelElement = GalerkinElement::fromPatch(patch);
    leaf = topLevelElement->regularLeafAtPoint(&u, &v);

    rad = GalerkinBasis::radianceAtPoint(leaf, leaf->radiance, u, v);

    if ( galerkinState.useAmbientRadiance ) {
        // Add ambient radiance
        ColorRgb reflectivity = patch->radianceData->Rd;
        ColorRgb ambientRadiance;
        ambientRadiance.scalarProduct(reflectivity, galerkinState.ambientRadiance);
        rad.add(rad, ambientRadiance);
    }

    return rad;
}

/**
Radiance data for a Patch is a surface element
*/
Element *
GalerkinRadianceMethod::createPatchData(Patch *patch) {
    return patch->radianceData = new GalerkinElement(patch, &galerkinState);
}

void
GalerkinRadianceMethod::destroyPatchData(Patch *patch) {
    if ( patch == nullptr ) {
        return;
    }
    if ( patch->radianceData != nullptr ) {
        delete static_cast<GalerkinElement *>(patch->radianceData);
        patch->radianceData = nullptr;
    }
}

char *
GalerkinRadianceMethod::getStats() {
    static char stats[STRING_LENGTH]{};

    for ( int i = 0 ; i < STRING_LENGTH; i++ ) {
        stats[i] = '\0';
    }

    int statsOffset = 0;

    appendStatsText(stats, &statsOffset, "Galerkin Radiosity Statistics:\n\n");
    appendStatsText(stats, &statsOffset, "Iteration: %d\n\n", galerkinState.iterationNumber);
    appendStatsText(stats, &statsOffset, "Nr. elements: %d\n", GalerkinElement::getNumberOfElements());
    appendStatsText(stats, &statsOffset, "clusters: %d\n", GalerkinElement::getNumberOfClusters());
    appendStatsText(stats, &statsOffset, "surface elements: %d\n\n", GalerkinElement::getNumberOfSurfaceElements());
    appendStatsText(stats, &statsOffset, "Nr. interactions: %d\n", Interaction::getNumberOfInteractions());
    appendStatsText(stats, &statsOffset, "cluster to cluster: %d\n", Interaction::getNumberOfClusterToClusterInteractions());
    appendStatsText(stats, &statsOffset, "cluster to surface: %d\n", Interaction::getNumberOfClusterToSurfaceInteractions());
    appendStatsText(stats, &statsOffset, "surface to cluster: %d\n", Interaction::getNumberOfSurfaceToClusterInteractions());
    appendStatsText(stats, &statsOffset, "surface to surface: %d\n", Interaction::getNumberOfSurfaceToSurfaceInteractions());
    appendStatsText(stats, &statsOffset, "shadow hits: %d\n", GLOBAL_statistics.numberOfShadowRays);
    appendStatsText(stats, &statsOffset, "shadow hits cached: %d\n", GLOBAL_statistics.numberOfShadowCacheHits);
    appendStatsText(stats, &statsOffset, "CPU time: %g secs.\n", galerkinState.cpuSeconds);
    appendStatsText(stats, &statsOffset, "Minimum element area: %g m^2\n", GLOBAL_statistics.totalArea * static_cast<double>(galerkinState.relMinElemArea));
    appendStatsText(stats, &statsOffset, "Link error threshold: %g %s\n\n",
         (galerkinState.errorNorm == RADIANCE_ERROR ?
                   M_PI * (galerkinState.relLinkErrorThreshold *
                           GLOBAL_statistics.maxSelfEmittedRadiance.luminance()) :
                   galerkinState.relLinkErrorThreshold *
                   GLOBAL_statistics.maxSelfEmittedPower.luminance()),
         (galerkinState.errorNorm == RADIANCE_ERROR ? "lux" : "lumen"));

    return stats;
}

void
GalerkinRadianceMethod::renderScene(const Scene *scene, const RenderOptions *renderOptions) const {
    if ( renderOptions->frustumCulling ) {
        Opengl::openGlRenderWorldOctree(scene, galerkinRenderPatch, renderOptions);
    } else {
        RenderOptions modifiedRenderOptions = *renderOptions;
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            if ( GLOBAL_render_glutDebugState.showSelectedPathOnly ) {
                if ( i == GLOBAL_render_glutDebugState.selectedPatch ) {
                    modifiedRenderOptions.drawOutlines = true;
                    modifiedRenderOptions.outlineColor = ColorRgb(1.0f, 0.0f, 0.0f);
                } else {
                    modifiedRenderOptions.drawOutlines = false;
                }
                galerkinRenderPatch(scene->patchList->get(i), scene->camera, &modifiedRenderOptions);
            } else {
                modifiedRenderOptions.outlineColor = ColorRgb(0.4f, 0.1f, 0.1f);
                if ( i == GLOBAL_render_glutDebugState.selectedPatch ) {
                    modifiedRenderOptions.outlineColor = ColorRgb(0.0f, 0.0f, 1.0f);
                }
                galerkinRenderPatch(scene->patchList->get(i), scene->camera, &modifiedRenderOptions);
            }
        }
    }
}

void
GalerkinRadianceMethod::writeVRML(
    const Camera *camera,
    java::io::OutputStream *outputStream,
    const RenderOptions *renderOptions) const
{
    if ( outputStream == nullptr ) {
        return;
    }
    VrmlWriter::writeHeader(camera, outputStream, renderOptions);

    vrmlOutputStream = outputStream;
    writeCoords();
    writeColors(renderOptions);
    writeCoordIndicesTopCluster();

    VrmlWriter::writeTrailer(outputStream);
}
