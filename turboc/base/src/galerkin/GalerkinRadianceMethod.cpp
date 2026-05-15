/**
Galerkin radiosity, with the following variants:
- With or without hierarchical refinement
- With or without clusters
- With Jacobi, Gauss-Seidel or South well iterations
- With potential-driven or not
*/

#include <string.h>

#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "common/color/Cie.h"
#include "tonemap/ToneMap.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/wrl/VrmlWriter.h"
#include "galerkin/GalerkinBasis.h"
#include "galerkin/GalerkinRadianceMethod.h"
#include "galerkin/processing/ClusterCreationStrategy.h"
#include "galerkin/processing/GatheringClusteredStrategy.h"
#include "galerkin/processing/GatheringSimpleStrategy.h"
#include "galerkin/processing/ScratchVisibilityStrategy.h"
#include "galerkin/processing/ShootingStrategy.h"

GalerkinState GalerkinRadianceMethod::galerkinState;
OutputStream *GalerkinRadianceMethod::vrmlOutputStream = NULL;
int GalerkinRadianceMethod::numberOfWrites = 0;
int GalerkinRadianceMethod::vertexId = 0;

String
GalerkinRadianceMethod::formatToString(const char *format, va_list arguments) {
    if ( format == NULL ) {
        return String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = Formatter::vformat(localBuffer, ((int)(sizeof(localBuffer))), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return String();
    }
    if ( required < ((int)(sizeof(localBuffer))) ) {
        return String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

void
GalerkinRadianceMethod::writeFormatted(const char *format, ...) {
    if ( vrmlOutputStream == NULL || format == NULL ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    String text = GalerkinRadianceMethod::formatToString(format, arguments);
    va_end(arguments);

    if ( text.isEmpty() ) {
        return;
    }

    PersistenceElement::writeBytes(
        *vrmlOutputStream,
        ((const unsigned char *)(text.toCString())),
        text.length());
}

void
GalerkinRadianceMethod::appendStatsText(char *buffer, int *offset, const char *format, ...) {
    if ( *offset >= STRING_LENGTH - 1 ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    String text = GalerkinRadianceMethod::formatToString(format, arguments);
    va_end(arguments);

    const int written = text.length();
    if ( written <= 0 ) {
        return;
    }

    const int available = STRING_LENGTH - *offset - 1;
    const int copied = (written > available) ? available : written;
    if ( copied > 0 ) {
        memcpy(&buffer[*offset], text.toCString(), ((size_t)(copied)));
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
    const GalerkinElement *galerkinElement = ((GalerkinElement *)(element));
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
GalerkinRadianceMethod::writeVertexColor(const ColorRgbMutable *color) {
    if ( numberOfWrites > 0 ) {
        writeFormatted("%s", ", ");
    }
    numberOfWrites++;
    if ( numberOfWrites % 4 == 0 ) {
        writeFormatted("%s", "\n\t  ");
    }
    writeFormatted("%.3g %.3g %.3g", color->getR(), color->getG(), color->getB());
    vertexId++;
}

void
GalerkinRadianceMethod::writeVertexColors(Element *element) {
    const GalerkinElement *galerkinElement = ((GalerkinElement *)(element));
    ColorRgbMutable vertexRadiosity[4];
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
        ColorRgbMutable reflectivity = galerkinElement->patch->radianceData->Rd;
        ColorRgbMutable ambient;

        ambient.scalarProduct(reflectivity, GalerkinRadianceMethod::galerkinState.ambientRadiance);
        for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
            vertexRadiosity[i].add(vertexRadiosity[i], ambient);
        }
    }

    for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
        ColorRgb col;
        ToneMap::radianceToRgb(ColorRgb(vertexRadiosity[i]), &col, *GalerkinRadianceMethod::galerkinState.toneMapOptions);
        ColorRgbMutable out(col.getR(), col.getG(), col.getB());
        writeVertexColor(&out);
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
    if ( !renderOptions->isSmoothShading() ) {
        Logger::warning(NULL, "I assume you want a smooth shaded model ...");
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
    const GalerkinElement *galerkinElement = ((GalerkinElement *)(element));
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
    if ( GalerkinRadianceMethod::galerkinState.scratch != NULL ) {
        delete GalerkinRadianceMethod::galerkinState.scratch;
        GalerkinRadianceMethod::galerkinState.scratch = NULL;
    }
}

GalerkinRadianceMethod::GalerkinRadianceMethod() {
    className = GALERKIN;
    gatheringStrategy = NULL;
}

GalerkinRadianceMethod::~GalerkinRadianceMethod() {
    if ( galerkinState.topCluster != NULL ) {
        galerkinState.topCluster = NULL;
    }
    if ( gatheringStrategy != NULL ) {
        delete gatheringStrategy;
        gatheringStrategy = NULL;
    }
}

/**
For counting how much CPU time was used for the computations
*/
void
GalerkinRadianceMethod::updateCpuSecs() {
    const long t = System::nanoTime();
    galerkinState.cpuSeconds += ((float)(((double)(t - galerkinState.lastClock)) / 1000000000.0));
    galerkinState.lastClock = t;
}

void
GalerkinRadianceMethod::patchInit(Patch *patch) {
    ColorRgbMutable reflectivity = patch->radianceData->Rd;
    ColorRgbMutable selfEmittanceRadiance = patch->radianceData->Ed;

    if ( galerkinState.useConstantRadiance ) {
        // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
        // '95, Dublin, Ireland, June 1995, p 336-344
        galerkinGetRadiance(patch).scalarProduct(reflectivity, galerkinState.constantRadiance);
        galerkinGetRadiance(patch).add(galerkinGetRadiance(patch), selfEmittanceRadiance);
        if ( galerkinState.galerkinIterationMethod == SOUTH_WELL ) {
            patch->radianceData->unShotRadiance[0].subtract(galerkinGetRadiance(patch), galerkinState.constantRadiance);
        }
    } else {
        galerkinSetRadiance(patch, selfEmittanceRadiance);
        if ( galerkinState.galerkinIterationMethod == SOUTH_WELL ) {
            patch->radianceData->unShotRadiance[0] = galerkinGetRadiance(patch);
        }
    }

    if ( galerkinState.importanceDriven ) {
        switch ( galerkinState.galerkinIterationMethod ) {
            case GAUSS_SEIDEL:
            case JACOBI:
                galerkinSetPotential(patch, patch->directPotential);
                break;
            case SOUTH_WELL:
                galerkinSetPotential(patch, patch->directPotential);
                galerkinSetUnShotPotential(patch, patch->directPotential);
                break;
            default:
                Logger::fatal(-1, "patchInit", "Invalid iteration method");
        }
    }

    recomputePatchColor(patch);
}

/**
Recomputes the color of a patch using ambient radiance term, ... if requested for
*/
void
GalerkinRadianceMethod::recomputePatchColor(Patch *patch) {
    ColorRgbMutable reflectivity = patch->radianceData->Rd;
    ColorRgbMutable radVis;

    // Compute the patches color based on its radiance + ambient radiance if desired
    if ( galerkinState.useAmbientRadiance ) {
        radVis.scalarProduct(reflectivity, galerkinState.ambientRadiance);
        radVis.add(radVis, galerkinGetRadiance(patch));
        ToneMap::radianceToRgb(ColorRgb(radVis), &patch->color, *galerkinState.toneMapOptions);
    } else {
        ToneMap::radianceToRgb(ColorRgb(galerkinGetRadiance(patch)), &patch->color, *galerkinState.toneMapOptions);
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
GalerkinRadianceMethod::initialize(Scene *scene, ToneMappingContext *toneMapOptions) {
    galerkinState.toneMapOptions = toneMapOptions;
    if ( galerkinState.toneMapOptions == NULL ) {
        Logger::fatal(-1, "GalerkinRadianceMethod::initialize", "Tone mapping context not provided");
    }

    galerkinState.iterationNumber = 0;
    galerkinState.cpuSeconds = 0.0;

    GalerkinElement::initializeBasis();

    galerkinState.constantRadiance = ColorRgbMutable(Statistics::instance().radiance.estimatedAverageRadiance.getR(), Statistics::instance().radiance.estimatedAverageRadiance.getG(), Statistics::instance().radiance.estimatedAverageRadiance.getB());
    if ( galerkinState.useConstantRadiance ) {
        galerkinState.ambientRadiance = ColorRgbMutable(0.0f, 0.0f, 0.0f);
    } else {
        galerkinState.ambientRadiance = ColorRgbMutable(Statistics::instance().radiance.estimatedAverageRadiance.getR(), Statistics::instance().radiance.estimatedAverageRadiance.getG(), Statistics::instance().radiance.estimatedAverageRadiance.getB());
    }

    for ( int i = 0; scene->patchList != NULL && i < scene->patchList->size(); i++ ) {
        patchInit(scene->patchList->get(i));
    }

    galerkinState.topCluster = ClusterCreationStrategy::createClusterHierarchy(
        scene->clusteredRootGeometry, &galerkinState);

    // Create a scratch software renderer for various operations on clusters
    if ( galerkinState.clusteringStrategy == Z_VISIBILITY ) {
        ScratchVisibilityStrategy::scratchInit(&galerkinState);
    }

    // Global variables for scratch rendering
    galerkinState.lastClusterId = -1;
    galerkinState.lastEye.set(Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE);
}

bool
GalerkinRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    if ( galerkinState.iterationNumber < 0 ) {
        Logger::error("doGalerkinOneStep", "method not initialized");
        return true; // Done, don't continue!
    }

    galerkinState.iterationNumber++;
    galerkinState.lastClock = System::nanoTime();

    // And now the real work
    int done = 1;

    switch ( galerkinState.galerkinIterationMethod ) {
        case JACOBI:
        case GAUSS_SEIDEL:
            done = gatheringStrategy->doGatheringIteration(scene, &galerkinState, renderOptions);
            break;
        case SOUTH_WELL:
            done = ShootingStrategy::doShootingStep(scene, &galerkinState, renderOptions);
            break;
        default:
            Logger::fatal(2, "doGalerkinOneStep", "Invalid iteration method %d\n", galerkinState.galerkinIterationMethod);
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

    for ( int i = 0; clusterElement->irregularSubElements != NULL && i < clusterElement->irregularSubElements->size(); i++ ) {
        GalerkinRadianceMethod::galerkinDestroyClusterHierarchy(((GalerkinElement *)(clusterElement->irregularSubElements->get(i))));
    }
    delete clusterElement;
}

void
GalerkinRadianceMethod::terminate(ArrayList<Patch *> */*scenePatches*/) {
    if ( galerkinState.clusteringStrategy == Z_VISIBILITY ) {
        ScratchVisibilityStrategy::scratchTerminate(&galerkinState);
    }
    if ( galerkinState.topCluster != NULL ) {
        GalerkinRadianceMethod::galerkinDestroyClusterHierarchy(galerkinState.topCluster);
        delete galerkinState.topCluster;
        galerkinState.topCluster = NULL;
    }
}

ColorRgbMutable
GalerkinRadianceMethod::computePatchRadiance(Patch *patch, double u, double v) const
{
    const GalerkinElement *leaf;
    ColorRgbMutable rad;

    if ( patch->jacobian ) {
        patch->biLinearToUniform(&u, &v);
    }

    GalerkinElement *topLevelElement = GalerkinElement::fromPatch(patch);
    leaf = topLevelElement->regularLeafAtPoint(&u, &v);

    rad = GalerkinBasis::radianceAtPoint(leaf, leaf->radiance, u, v);

    if ( galerkinState.useAmbientRadiance ) {
        // Add ambient radiance
        ColorRgbMutable reflectivity = patch->radianceData->Rd;
        ColorRgbMutable ambientRadiance;
        ambientRadiance.scalarProduct(reflectivity, galerkinState.ambientRadiance);
        rad.add(rad, ambientRadiance);
    }

    return rad;
}

ColorRgbMutable
GalerkinRadianceMethod::getRadiance(
    Camera * /*camera*/,
    Patch *patch,
    double u,
    double v,
    Vector3D /*dir*/,
    const RenderOptions * /*renderOptions*/) const
{
    return computePatchRadiance(patch, u, v);
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
    if ( patch == NULL ) {
        return;
    }
    if ( patch->radianceData != NULL ) {
        delete ((GalerkinElement *)(patch->radianceData));
        patch->radianceData = NULL;
    }
}

char *
GalerkinRadianceMethod::getStats() const {
    static char stats[STRING_LENGTH] = {0};

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
    appendStatsText(stats, &statsOffset, "cluster to cluster: %d\n", Interaction::getNumOClustTClustInters());
    appendStatsText(stats, &statsOffset, "cluster to surface: %d\n", Interaction::getNumOClustTSurfInters());
    appendStatsText(stats, &statsOffset, "surface to cluster: %d\n", Interaction::getNumOSurfTClustInters());
    appendStatsText(stats, &statsOffset, "surface to surface: %d\n", Interaction::getNumOSurfTSurfInters());
    appendStatsText(stats, &statsOffset, "shadow hits: %d\n", Statistics::instance().shadow.numberOfShadowRays);
    appendStatsText(stats, &statsOffset, "shadow hits cached: %d\n", Statistics::instance().shadow.numberOfShadowCacheHits);
    appendStatsText(stats, &statsOffset, "CPU time: %g secs.\n", galerkinState.cpuSeconds);
    appendStatsText(stats, &statsOffset, "Minimum element area: %g m^2\n", Statistics::instance().radiance.totalArea * ((double)(galerkinState.relMinElemArea)));
    appendStatsText(stats, &statsOffset, "Link error threshold: %g %s\n\n",
         (galerkinState.errorNorm == RADIANCE_ERROR ?
                   M_PI * (galerkinState.relLinkErrorThreshold *
                           Cie::spectrumLuminance(Statistics::instance().radiance.maxSelfEmittedRadiance.getR(), Statistics::instance().radiance.maxSelfEmittedRadiance.getG(), Statistics::instance().radiance.maxSelfEmittedRadiance.getB())) :
                   galerkinState.relLinkErrorThreshold *
                   Cie::spectrumLuminance(Statistics::instance().radiance.maxSelfEmittedPower.getR(), Statistics::instance().radiance.maxSelfEmittedPower.getG(), Statistics::instance().radiance.maxSelfEmittedPower.getB())),
         (galerkinState.errorNorm == RADIANCE_ERROR ? "lux" : "lumen"));

    return stats;
}

void
GalerkinRadianceMethod::writeVRML(
    const Camera *camera,
    OutputStream *outputStream,
    const RenderOptions *renderOptions) const
{
    if ( outputStream == NULL ) {
        return;
    }
    VrmlWriter::writeHeader(camera, outputStream, renderOptions);

    vrmlOutputStream = outputStream;
    writeCoords();
    writeColors(renderOptions);
    writeCoordIndicesTopCluster();

    VrmlWriter::writeTrailer(outputStream);
}
