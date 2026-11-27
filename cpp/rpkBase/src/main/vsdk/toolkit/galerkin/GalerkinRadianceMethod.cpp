/**
Galerkin radiosity, with the following variants:
- With or without hierarchical refinement
- With or without clusters
- With Jacobi, Gauss-Seidel or South well iterations
- With potential-driven or not
*/

#include <cstring>

#include "java/lang/Double.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/io/wrapper/PersistenceElement.h"
#include "vsdk/toolkit/io/wrl/VrmlWriter.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/GalerkinRadianceMethod.h"
#include "vsdk/toolkit/galerkin/processing/ClusterCreationStrategy.h"
#include "vsdk/toolkit/galerkin/processing/GatheringClusteredStrategy.h"
#include "vsdk/toolkit/galerkin/processing/GatheringSimpleStrategy.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityStrategy.h"
#include "vsdk/toolkit/galerkin/processing/ShootingStrategy.h"

GalerkinState GalerkinRadianceMethod::galerkinState;
java::OutputStream *GalerkinRadianceMethod::vrmlOutputStream = nullptr;
int GalerkinRadianceMethod::numberOfWrites = 0;
int GalerkinRadianceMethod::vertexId = 0;

java::String
GalerkinRadianceMethod::formatToString(const char *format, va_list arguments) {
    if ( format == nullptr ) {
        return java::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return java::String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    java::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    java::String result(dynamicBuffer);
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
    java::String text = GalerkinRadianceMethod::formatToString(format, arguments);
    va_end(arguments);

    if ( text.isEmpty() ) {
        return;
    }

    PersistenceElement::writeBytes(
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
    java::String text = GalerkinRadianceMethod::formatToString(format, arguments);
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
GalerkinRadianceMethod::writeVertexCoord(const Vector3Dd *p) {
    if ( numberOfWrites > 0 ) {
        writeFormatted("%s", ", ");
    }
    numberOfWrites++;
    if ( numberOfWrites % 4 == 0 ) {
        writeFormatted("%s", "\n\t  ");
    }
    writeFormatted("%g %g %g", p->x(), p->y(), p->z());
    vertexId++;
}

void
GalerkinRadianceMethod::writeVertexCoords(Element *element) {
    const GalerkinElement *galerkinElement = static_cast<GalerkinElement *>(element);
    Vector3Dd v[8];
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
GalerkinRadianceMethod::writeVertexColor(const ColorRgb &color) {
    if ( numberOfWrites > 0 ) {
        writeFormatted("%s", ", ");
    }
    numberOfWrites++;
    if ( numberOfWrites % 4 == 0 ) {
        writeFormatted("%s", "\n\t  ");
    }
    writeFormatted("%.3g %.3g %.3g", color.getR(), color.getG(), color.getB());
    vertexId++;
}

void
GalerkinRadianceMethod::writeVertexColors(Element *element) {
    const GalerkinElement *galerkinElement = static_cast<GalerkinElement *>(element);
    ColorRgb vertexRadiosity[4];
    int i;

    if ( galerkinElement->patch->getNumberOfVertices() == 3 ) {
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
        const ColorRgb &reflectivity = galerkinElement->galerkinState->topLevelElement(galerkinElement->patch)->Rd;
        const ColorRgb &ambientRadiance = GalerkinRadianceMethod::galerkinState.ambientRadiance;
        const ColorRgb ambient(reflectivity.getR() * ambientRadiance.getR(),
                               reflectivity.getG() * ambientRadiance.getG(),
                               reflectivity.getB() * ambientRadiance.getB());
        for ( i = 0; i < galerkinElement->patch->getNumberOfVertices(); i++ ) {
            vertexRadiosity[i] = vertexRadiosity[i].add(ambient);
        }
    }

    for ( i = 0; i < galerkinElement->patch->getNumberOfVertices(); i++ ) {
        const ColorRgb col = ToneMap::radianceToRgb(vertexRadiosity[i], *GalerkinRadianceMethod::galerkinState.toneMapOptions);
        writeVertexColor(col);
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
GalerkinRadianceMethod::writeColors(const RendererConfiguration *renderOptions) {
    if ( !renderOptions->isSmoothShading() ) {
        Logger::reportMessage("GalerkinRadianceMethod", Logger::WARNING, "", "I assume you want a smooth shaded model ...");
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
    for ( int i = 0; i < galerkinElement->patch->getNumberOfVertices(); i++ ) {
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
    ScratchVisibilityStrategy::scratchTerminate(&GalerkinRadianceMethod::galerkinState);
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

/**
For counting how much CPU time was used for the computations
*/
void
GalerkinRadianceMethod::updateCpuSecs() {
    const long long t = java::System::nanoTime();
    galerkinState.cpuSeconds += static_cast<double>(static_cast<double>(t - galerkinState.lastClock) / 1000000000.0);
    galerkinState.lastClock = t;
}

void
GalerkinRadianceMethod::patchInit(Patch *patch, const DirectPotentialTable *directPotentials, FaceColorTable *faceColors) {
    if ( galerkinState.useConstantRadiance ) {
        // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
        // '95, Dublin, Ireland, June 1995, p 336-344
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
            galerkinState.topLevelElement(patch)->unShotRadiance[0] =
                galerkinGetRadiance(patch).subtract(galerkinState.constantRadiance);
        }
    } else {
        galerkinSetRadiance(patch, galerkinState.topLevelElement(patch)->Ed);
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
            galerkinState.topLevelElement(patch)->unShotRadiance[0] = galerkinGetRadiance(patch);
        }
    }

    if ( galerkinState.importanceDriven ) {
        switch ( galerkinState.galerkinIterationMethod ) {
            case GalerkinIterationMethod::GAUSS_SEIDEL:
            case GalerkinIterationMethod::JACOBI:
            case GalerkinIterationMethod::PARALLEL_JACOBI:
                galerkinSetPotential(patch, directPotentials->get(patch->getFaceIndex()));
                break;
            case GalerkinIterationMethod::SOUTH_WELL:
                galerkinSetPotential(patch, directPotentials->get(patch->getFaceIndex()));
                galerkinSetUnShotPotential(patch, directPotentials->get(patch->getFaceIndex()));
                break;
            default:
                Logger::reportMessage("GalerkinRadianceMethod", Logger::FATAL_ERROR, "patchInit", "Invalid iteration method");
        }
    }

    recomputePatchColor(patch, faceColors);
}

/**
Recomputes the color of a patch using ambient radiance term, ... if requested for
*/
void
GalerkinRadianceMethod::recomputePatchColor(Patch *patch, FaceColorTable *faceColors) {
    const ColorRgb reflectivity = galerkinState.topLevelElement(patch)->Rd;

    // Compute the patches color based on its radiance + ambient radiance if desired
    if ( galerkinState.useAmbientRadiance ) {
        const ColorRgb radianceWithAmbient = reflectivity.multiply(galerkinState.ambientRadiance).add(galerkinGetRadiance(patch));
        const ColorRgb patchColor = ToneMap::radianceToRgb(radianceWithAmbient, *galerkinState.toneMapOptions);
        faceColors->set(patch->getFaceIndex(), patchColor);
    } else {
        const ColorRgb patchColor = ToneMap::radianceToRgb(galerkinGetRadiance(patch), *galerkinState.toneMapOptions);
        faceColors->set(patch->getFaceIndex(), patchColor);
    }
    patch->computeVertexColors(faceColors);
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
    galerkinState.setRadianceElements(scene->getRadianceElements());
    galerkinState.toneMapOptions = toneMapOptions;
    if ( galerkinState.toneMapOptions == nullptr ) {
        Logger::reportMessage("GalerkinRadianceMethod", Logger::FATAL_ERROR, "GalerkinRadianceMethod::initialize", "Tone mapping context not provided");
    }

    galerkinState.iterationNumber = 0;
    galerkinState.cpuSeconds = 0.0;

    GalerkinElement::initializeBasis();

    galerkinState.constantRadiance = Statistics::instance().radiance.estimatedAverageRadiance;
    if ( galerkinState.useConstantRadiance ) {
        galerkinState.ambientRadiance = ColorRgb(0.0, 0.0, 0.0);
    } else {
        galerkinState.ambientRadiance = Statistics::instance().radiance.estimatedAverageRadiance;
    }

    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        patchInit(scene->patchList->get(i), scene->getDirectPotentials(), scene->getFaceColors());
    }

    galerkinState.topCluster = ClusterCreationStrategy::createClusterHierarchy(
        scene->clusteredRootGeometry, &galerkinState);

    // Create a scratch software renderer for various operations on clusters
    if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy::Z_VISIBILITY ) {
        ScratchVisibilityStrategy::scratchInit(&galerkinState);
    }
}

bool
GalerkinRadianceMethod::doStep(Scene *scene, RendererConfiguration *renderOptions) {
    if ( galerkinState.iterationNumber < 0 ) {
        Logger::reportMessage("GalerkinRadianceMethod", Logger::ERROR, "doGalerkinOneStep", "method not initialized");
        return true; // Done, don't continue!
    }

    galerkinState.iterationNumber++;
    galerkinState.lastClock = java::System::nanoTime();

    // And now the real work
    int done = 1;

    switch ( galerkinState.galerkinIterationMethod ) {
        case GalerkinIterationMethod::JACOBI:
        case GalerkinIterationMethod::PARALLEL_JACOBI:
        case GalerkinIterationMethod::GAUSS_SEIDEL:
            done = gatheringStrategy->doGatheringIteration(scene, &galerkinState, renderOptions);
            break;
        case GalerkinIterationMethod::SOUTH_WELL:
            done = ShootingStrategy::doShootingStep(scene, &galerkinState, renderOptions);
            break;
        default:
            Logger::reportMessage("GalerkinRadianceMethod", Logger::FATAL_ERROR, "doGalerkinOneStep", java::String::format("Invalid iteration method %d\n", galerkinState.galerkinIterationMethod));
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
GalerkinRadianceMethod::computePatchRadiance(const Patch *patch, double u, double v) const
{
    const GalerkinElement *leaf;

    if ( patch->getFace().hasJacobian() ) {
        patch->biLinearToUniform(&u, &v);
    }

    GalerkinElement *topLevelElement = GalerkinElement::fromPatch(&galerkinState, patch);
    leaf = topLevelElement->regularLeafAtPoint(&u, &v);

    const ColorRgb rad = GalerkinBasis::radianceAtPoint(leaf, leaf->radiance, u, v);

    if ( galerkinState.useAmbientRadiance ) {
        // Add ambient radiance
        const ColorRgb &reflectivity = galerkinState.topLevelElement(patch)->Rd;
        const ColorRgb &ambient = galerkinState.ambientRadiance;
        return ColorRgb(rad.getR() + reflectivity.getR() * ambient.getR(),
                        rad.getG() + reflectivity.getG() * ambient.getG(),
                        rad.getB() + reflectivity.getB() * ambient.getB());
    }

    return rad;
}

ColorRgb
GalerkinRadianceMethod::getRadiance(
    Camera * /*camera*/,
    const Patch *patch,
    double u,
    double v,
    Vector3Dd /*dir*/,
    const RendererConfiguration * /*renderOptions*/) const
{
    return computePatchRadiance(patch, u, v);
}

/**
Radiance data for a Patch is a surface element
*/
Element *
GalerkinRadianceMethod::createPatchData(Patch *patch) {
    return new GalerkinElement(patch, &galerkinState);
}

void
GalerkinRadianceMethod::destroyPatchData(Patch *patch) {
    if ( patch == nullptr ) {
        return;
    }
    // Elements are bound once the method has been initialized
    if ( galerkinState.getRadianceElements() != nullptr ) {
        delete galerkinState.getRadianceElements()->get(patch->getFaceIndex());
    }
}

char *
GalerkinRadianceMethod::getStats() const {
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
    appendStatsText(stats, &statsOffset, "shadow hits: %d\n", Statistics::instance().shadow.numberOfShadowRays);
    appendStatsText(stats, &statsOffset, "shadow hits cached: %d\n", Statistics::instance().shadow.numberOfShadowCacheHits);
    appendStatsText(stats, &statsOffset, "CPU time: %g secs.\n", galerkinState.cpuSeconds);
    appendStatsText(stats, &statsOffset, "Minimum element area: %g m^2\n", Statistics::instance().radiance.totalArea * static_cast<double>(galerkinState.relMinElemArea));
    appendStatsText(stats, &statsOffset, "Link error threshold: %g %s\n\n",
         (galerkinState.errorNorm == RADIANCE_ERROR ?
                   M_PI * (galerkinState.relLinkErrorThreshold *
                           galerkinState.toneMapOptions->getColorSpace().spectrumLuminance(Statistics::instance().radiance.maxSelfEmittedRadiance.getR(), Statistics::instance().radiance.maxSelfEmittedRadiance.getG(), Statistics::instance().radiance.maxSelfEmittedRadiance.getB())) :
                   galerkinState.relLinkErrorThreshold *
                   galerkinState.toneMapOptions->getColorSpace().spectrumLuminance(Statistics::instance().radiance.maxSelfEmittedPower.getR(), Statistics::instance().radiance.maxSelfEmittedPower.getG(), Statistics::instance().radiance.maxSelfEmittedPower.getB())),
         (galerkinState.errorNorm == RADIANCE_ERROR ? "lux" : "lumen"));

    return stats;
}

void
GalerkinRadianceMethod::writeVRML(
    const Camera *camera,
    java::OutputStream *outputStream,
    const RendererConfiguration *renderOptions) const
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
