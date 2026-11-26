#include "vsdk/toolkit/skin/Compound.h"
#include <cstring>

#include "java/io/FileInputStream.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/scene/PatchClusterOctreeNode.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/numericalAnalysis/MeshSurfaceVisitor.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelDeserializer.h"
#include "vsdk/toolkit/io/bin/writer/BinaryModelSerializer.h"
#ifdef MGF_ENABLED
    #include "vsdk/toolkit/io/mgf/MgfParserLoader.h"
#endif
#include "vsdk/toolkit/render/RenderHookList.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "Adaptation.h"
#include "Batch.h"
#include "options/OptionsGroupCore.h"
#include "Radiance.h"
#include "SceneBuilder.h"

void
SceneBuilder::sceneBuilderPatchAccumulateStats(Patch *patch, double *powerSum, double *reflectivitySum) {
    const ColorRgb E = PatchVisitor::averageEmittance(patch, XxdfComponentFlagInfo::ALL_COMPONENTS);
    const ColorRgb R = PatchVisitor::averageNormalAlbedo(patch, BsdfComponentInfo::BSDF_ALL_COMPONENTS);
    const double area = patch->getArea();
    const ColorRgb power(area * E.getR(), area * E.getG(), area * E.getB());

    Statistics::instance().radiance.totalArea += patch->getArea();
    powerSum[0] += power.getR();
    powerSum[1] += power.getG();
    powerSum[2] += power.getB();
    reflectivitySum[0] += area * R.getR();
    reflectivitySum[1] += area * R.getG();
    reflectivitySum[2] += area * R.getB();
    Statistics::instance().radiance.maxSelfEmittedRadiance = E.multiply(1.0 / static_cast<double>(M_PI)).maximum(Statistics::instance().radiance.maxSelfEmittedRadiance);
    Statistics::instance().radiance.maxSelfEmittedPower = power.maximum(Statistics::instance().radiance.maxSelfEmittedPower);
}

void
SceneBuilder::sceneBuilderComputeStats(Scene *scene) {
    Vector3Dd zero;
    double powerSum[3] = {0.0, 0.0, 0.0};
    double reflectivitySum[3] = {0.0, 0.0, 0.0};
    zero = Vector3Dd(0, 0, 0);

    // Initialize
    Statistics::instance().radiance.totalEmittedPower = ColorRgb(0.0, 0.0, 0.0);
    Statistics::instance().radiance.averageReflectivity = ColorRgb(0.0, 0.0, 0.0);
    Statistics::instance().radiance.maxSelfEmittedRadiance = ColorRgb(0.0, 0.0, 0.0);
    Statistics::instance().radiance.maxSelfEmittedPower = ColorRgb(0.0, 0.0, 0.0);
    Statistics::instance().radiance.totalArea = 0.0;

    // Accumulate
    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        SceneBuilder::sceneBuilderPatchAccumulateStats(scene->patchList->get(i), powerSum, reflectivitySum);
    }

    // Averages
    Statistics::instance().radiance.totalEmittedPower = ColorRgb(powerSum[0], powerSum[1], powerSum[2]);
    Statistics::instance().radiance.averageReflectivity = ColorRgb(reflectivitySum[0], reflectivitySum[1], reflectivitySum[2]).scaleInverse(Statistics::instance().radiance.totalArea);
    const ColorRgb &reflectivity = Statistics::instance().radiance.averageReflectivity;
    const ColorRgb averageAbsorption(1.0 - reflectivity.getR(), 1.0 - reflectivity.getG(), 1.0 - reflectivity.getB());
    Statistics::instance().radiance.estimatedAverageRadiance = Statistics::instance().radiance.totalEmittedPower.scaleInverse(static_cast<double>(M_PI) * Statistics::instance().radiance.totalArea);

    // Include background radiation
    const ColorRgb BP = Background::backgroundPower(scene->background, &zero).multiply(1.0 / (4.0 * static_cast<double>(M_PI)));
    Statistics::instance().radiance.totalEmittedPower = Statistics::instance().radiance.totalEmittedPower.add(BP);
    Statistics::instance().radiance.estimatedAverageRadiance = Statistics::instance().radiance.estimatedAverageRadiance.add(BP).divide(averageAbsorption);

    Statistics::instance().potential.totalDirectPotential = 0.0;
    Statistics::instance().potential.maxDirectPotential = 0.0;
    Statistics::instance().potential.averageDirectPotential = 0.0;
    Statistics::instance().potential.maxDirectImportance = 0.0;
}

/**
Adds the background to the global light source patch list
*/
void
SceneBuilder::sceneBuilderAddBackgroundToLightSourceList(Scene *scene) {
    if ( scene->background != nullptr && scene->background->bkgPatch != nullptr ) {
        scene->lightSourcePatchList->add(scene->background->bkgPatch);
        Statistics::instance().reader.numberOfLightSources++;
    }
}

/**
Adds the patch to the global light source patch list if the patch is on
a light source (i.e. when the surfaces material has a non-null edf)
*/
void
SceneBuilder::sceneBuilderAddPatchToLightSourceListIfLightSource(java::ArrayList<Patch *> *lights, Patch *patch) {
    if ( patch != nullptr
         && patch->getMaterial() != nullptr
         && patch->getMaterial()->getEdf() != nullptr ) {
        lights->add(patch);
        Statistics::instance().reader.numberOfLightSources++;
    }
}

/**
Build the global light source patch list
*/
void
SceneBuilder::sceneBuilderFillLightSourcePatchList(Scene *scene) {
    java::ArrayList<Patch *> * const lights = new java::ArrayList<Patch *>();
    Statistics::instance().reader.numberOfLightSources = 0;

    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        SceneBuilder::sceneBuilderAddPatchToLightSourceListIfLightSource(lights, scene->patchList->get(i));
    }

    SceneBuilder::sceneBuilderAddBackgroundToLightSourceList(scene);
    scene->lightSourcePatchList = lights;
}

/**
Creates a hierarchical model of the discrete scene (the patches in the scene) using the simple
algorithm described in
- Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
  PhD Thesis, University of Washington, 1995, p 116
This hierarchy is often much more efficient for tracing rays and clustering radiosity algorithms
than the given hierarchy of bounding boxes. A pointer to the toplevel "cluster" is returned
*/
RpkGeometry *
SceneBuilder::sceneBuilderCreateClusterHierarchy(const java::ArrayList<Patch *> *patches) {
    PatchClusterOctreeNode *rootCluster;
    RpkGeometry *rootGeometry;

    // Create a toplevel cluster containing (references to) all the patches in the scene
    rootCluster = new PatchClusterOctreeNode(patches);

    // Split the toplevel cluster recursively into sub-clusters
    rootCluster->splitCluster();
    //rootCluster->print(0);

    // Convert to a geometry hierarchy, disposing of clusters
    rootGeometry = rootCluster->convertClusterToGeometry();

    delete rootCluster;
    return rootGeometry;
}

/**
Builds a linear list of patches making up all the geometries in the list, whether
they are primitive or not
*/
void
SceneBuilder::sceneBuilderPatchList(const java::ArrayList<RpkGeometry *> *geometryList, java::ArrayList<Patch *> *patchList) {
    for ( int i = 0; i < geometryList->size(); i++ ) {
        RpkGeometry * const geometry = geometryList->get(i);
        if ( geometry->isCompound() ) {
            // Recursive case
            const Compound *compound = static_cast<const Compound *>(geometry);
            SceneBuilder::sceneBuilderPatchList(compound->children, patchList);
        } else {
            // Trivial case
            const java::ArrayList<Patch *> *patchesFromNonCompounds = RpkGeometry::patchListReference(geometry);

            for ( int j = 0; patchesFromNonCompounds != nullptr && j < patchesFromNonCompounds->size(); j++ ) {
                Patch * const patch = patchesFromNonCompounds->get(j);
                if ( patch != nullptr ) {
                    patchList->add(patch);
                }
            }
        }
    }
}

void
SceneBuilder::sceneBuilderFillFacesBackPointers(const java::ArrayList<RpkGeometry *> *geometryList) {
    if ( geometryList == nullptr ) {
        return;
    }
    for ( int i = 0; i < geometryList->size(); i++ ) {
        RpkGeometry * const geometry = geometryList->get(i);
        if ( geometry == nullptr ) {
            continue;
        }
        if ( geometry->isCompound() ) {
            const Compound *compound = static_cast<const Compound *>(geometry);
            SceneBuilder::sceneBuilderFillFacesBackPointers(compound->children);
            continue;
        }
        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            MeshSurfaceVisitor::fillFacesBackPointers(static_cast<MeshSurface *>(geometry));
        }
    }
}

void
SceneBuilder::sceneBuilderCollectGeometriesRecursive(
    const java::ArrayList<RpkGeometry *> *source,
    java::ArrayList<RpkGeometry *> *target)
{
    if ( source == nullptr || target == nullptr ) {
        return;
    }

    for ( int i = 0; i < source->size(); i++ ) {
        RpkGeometry * const geometry = source->get(i);
        if ( geometry == nullptr ) {
            continue;
        }

        bool alreadyInTarget = false;
        for ( int j = 0; j < target->size(); j++ ) {
            if ( target->get(j) == geometry ) {
                alreadyInTarget = true;
                break;
            }
        }
        if ( !alreadyInTarget ) {
            target->add(geometry);
        }

        if ( geometry->isCompound() ) {
            const Compound *compound = static_cast<const Compound *>(geometry);
            SceneBuilder::sceneBuilderCollectGeometriesRecursive(compound->children, target);
        }
    }
}

void
SceneBuilder::sceneBuilderApplyModelToMgfContext(ParseRuntimeContext *mgfContext, ParseSnapshotContext *mgfModel) {
    if ( mgfContext == nullptr || mgfModel == nullptr ) {
        return;
    }

    mgfContext->currentColor = mgfModel->currentColor;
    mgfContext->currentFaceList = mgfModel->currentFaceList;
    mgfContext->currentGeometryList = mgfModel->currentGeometryList;
    mgfContext->currentMaterialName = mgfModel->currentMaterialName;
    mgfContext->currentNormalList = mgfModel->currentNormalList;
    mgfContext->currentObjectName = mgfModel->currentObjectName;
    mgfContext->currentPointList = mgfModel->currentPointList;
    mgfContext->currentVertexList = mgfModel->currentVertexList;
    mgfContext->currentVertexName = mgfModel->currentVertexName;
    mgfContext->geometries = mgfModel->geometries;
    mgfContext->geometryStackHeadIndex = mgfModel->geometryStackHeadIndex;
    mgfContext->inComplex = mgfModel->inComplex;
    mgfContext->inSurface = mgfModel->inSurface;
    mgfContext->materials = mgfModel->materials;
    mgfContext->monochrome = mgfModel->monochrome;
    mgfContext->readerContext = mgfModel->readerContext;
    mgfContext->transformContext = mgfModel->transformContext;
    mgfContext->model = mgfModel;

    mgfContext->currentMaterial = nullptr;
    if ( mgfContext->materials != nullptr && mgfContext->currentMaterialName != nullptr ) {
        for ( int i = 0; i < mgfContext->materials->size(); i++ ) {
            RpkMaterial * const material = mgfContext->materials->get(i);
            if ( material != nullptr
                 && material->getName() != nullptr
                 && strcmp(material->getName(), mgfContext->currentMaterialName) == 0 ) {
                mgfContext->currentMaterial = material;
                break;
            }
        }
    }

    if ( mgfContext->allGeometries != nullptr ) {
        mgfContext->allGeometries->dispose();
        delete mgfContext->allGeometries;
    }
    mgfContext->allGeometries = new java::ArrayList<RpkGeometry *>();
    SceneBuilder::sceneBuilderCollectGeometriesRecursive(mgfModel->currentGeometryList, mgfContext->allGeometries);
    if ( mgfModel->geometries != mgfModel->currentGeometryList ) {
        SceneBuilder::sceneBuilderCollectGeometriesRecursive(mgfModel->geometries, mgfContext->allGeometries);
    }
}

void
SceneBuilder::removeEmptyMeshSurfaces(ParseRuntimeContext *mgfContext, java::ArrayList<RpkGeometry *> *geometryList) {
    for ( int i = 0; i < geometryList->size(); i++ ) {
        const RpkGeometry *geometry = geometryList->get(i);
        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            const MeshSurface *mesh = static_cast<const MeshSurface *>(geometry);
            if ( mesh->vertices->size() == 0 ) {
                for ( int j = 0; j < mgfContext->allGeometries->size(); j++ ) {
                    const RpkGeometry *deletionCandidate = mgfContext->allGeometries->get(j);
                    if ( deletionCandidate == geometry ) {
                        mgfContext->allGeometries->remove(j);
                        break;
                    }
                }
                delete geometry;
                geometryList->remove(i);
                i--;
            }
        }
    }
}

bool
SceneBuilder::sceneBuilderHasExtension(const char *fileName, const char *extension) {
    if ( fileName == nullptr || extension == nullptr ) {
        return false;
    }

    const size_t fileNameLength = strlen(fileName);
    const size_t extensionLength = strlen(extension);
    if ( fileNameLength < extensionLength ) {
        return false;
    }

    return strcasecmp(fileName + fileNameLength - extensionLength, extension) == 0;
}

char *
SceneBuilder::sceneBuilderBuildBinaryFallbackPath(const char *mgfFileName) {
    if ( !sceneBuilderHasExtension(mgfFileName, ".mgf") ) {
        return nullptr;
    }

    const size_t length = strlen(mgfFileName);
    char * const fallbackFileName = new char[length + 1];
    memcpy(fallbackFileName, mgfFileName, length + 1);
    memcpy(fallbackFileName + length - 4, ".bin", 5);

    return fallbackFileName;
}

bool
SceneBuilder::sceneBuilderIsReadableRegularFile(const char *fileName) {
    if ( fileName == nullptr || fileName[0] == '\0' ) {
        return false;
    }

    java::File file(fileName);
    if ( !file.exists() || !file.isFile() || !file.canRead() ) {
        return false;
    }

    java::FileInputStream input(fileName);
    const int firstByte = input.read();
    input.close();
    return firstByte >= 0;
}

bool
SceneBuilder::sceneBuilderValidateReadableFile(
    const char *fileName,
    const char *fileRole)
{
    java::File file(fileName);
    if ( !file.exists() ) {
        Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", java::String::format("Requested %s file '%s' does not exist", fileRole, fileName));
        return false;
    }
    if ( !file.isFile() ) {
        Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", java::String::format("Requested %s file '%s' is not a regular file", fileRole, fileName));
        return false;
    }
    if ( !file.canRead() ) {
        Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", java::String::format("Requested %s file '%s' is not readable", fileRole, fileName));
        return false;
    }

    java::FileInputStream input(fileName);
    const int firstByte = input.read();
    input.close();

    if ( firstByte < 0 ) {
        Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", java::String::format("Requested %s file '%s' is empty", fileRole, fileName));
        return false;
    }

    return true;
}

/**
Tries to read the scene in the given file. Returns false if not successful.
Returns true if successful
*/
bool
SceneBuilder::sceneBuilderReadFile(
    const char *fileName,
    ParseRuntimeContext *mgfContext,
    Scene *scene,
    ToneMappingContext &toneMapOptions)
{
    const BatchOptions *batchOptions = Batch::batchGetOptions();
    const bool importBinaryOption =
        batchOptions != nullptr
        && batchOptions->importBinary
        && batchOptions->binaryInputFilename != nullptr
        && batchOptions->binaryInputFilename[0] != '\0';
    const char *requestedInputName = importBinaryOption ? batchOptions->binaryInputFilename : fileName;

    bool readBinaryModel =
        importBinaryOption
        || SceneBuilder::sceneBuilderHasExtension(requestedInputName, ".bin");

    char *fallbackBinaryInputName = nullptr;
    const char *inputName = requestedInputName;

#ifdef MGF_ENABLED
    if ( !readBinaryModel
         && SceneBuilder::sceneBuilderHasExtension(requestedInputName, ".mgf")
         && !SceneBuilder::sceneBuilderIsReadableRegularFile(requestedInputName) ) {
        fallbackBinaryInputName = SceneBuilder::sceneBuilderBuildBinaryFallbackPath(requestedInputName);
        if ( fallbackBinaryInputName != nullptr
             && SceneBuilder::sceneBuilderIsReadableRegularFile(fallbackBinaryInputName) ) {
            readBinaryModel = true;
            inputName = fallbackBinaryInputName;
        }
    }
#else
    if ( !readBinaryModel ) {
        if ( SceneBuilder::sceneBuilderHasExtension(requestedInputName, ".mgf") ) {
            fallbackBinaryInputName = SceneBuilder::sceneBuilderBuildBinaryFallbackPath(requestedInputName);
            if ( fallbackBinaryInputName != nullptr
                 && SceneBuilder::sceneBuilderIsReadableRegularFile(fallbackBinaryInputName) ) {
                readBinaryModel = true;
                inputName = fallbackBinaryInputName;
            } else {
                java::System::err.printf(
                    "ERROR: MGF input requires MGF support. Rebuild with CMake flag '-DWITH_MGF=ON'.\n");
                java::System::err.flush();
                Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", java::String::format("Requested MGF input '%s' could not be loaded and fallback binary '%s' is not available.", requestedInputName, fallbackBinaryInputName != nullptr ? fallbackBinaryInputName : "(not derivable)"));
                delete[] fallbackBinaryInputName;
                return false;
            }
        } else {
            java::System::err.printf(
                "ERROR: Non-binary scene input requires MGF support. Rebuild with CMake flag '-DWITH_MGF=ON'.\n");
            java::System::err.flush();
            Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", "Only '.bin' input files are supported in this build.");
            return false;
        }
    }
#endif

    // Check whether the file can be opened/read
    if ( readBinaryModel && !SceneBuilder::sceneBuilderValidateReadableFile(inputName, "binary model") ) {
        delete[] fallbackBinaryInputName;
        return false;
    }

    if ( !readBinaryModel && fileName[0] != '#' ) {
        if ( !SceneBuilder::sceneBuilderValidateReadableFile(fileName, "scene") ) {
            delete[] fallbackBinaryInputName;
            return false;
        }
    }

    // Get current directory from the filename
    const unsigned long n = strlen(inputName) + 1;

    char * const currentDirectory = new char[n];
    java::Formatter::format(currentDirectory, static_cast<int>(n), "%s", inputName);
    char *slash = strrchr(currentDirectory, '/');
    if ( slash != nullptr ) {
        *slash = '\0';
    } else {
        *currentDirectory = '\0';
    }

    // Prepare if errors occur when reading the new scene will abort
    scene->geometryList = nullptr;

    Patch::setNextId(1);
    if ( scene->background != nullptr ) {
        delete scene->background;
    }
    scene->background = OptionsGroupCore::createBackground();

    // Read the source scene description into a ParseSnapshotContext snapshot
    java::System::err.printf("Reading the scene from file '%s' ... \n", inputName);
    long long last = java::System::nanoTime();
    ParseSnapshotContext *mgfModel = nullptr;

    if ( readBinaryModel ) {
        mgfModel = BinaryModelDeserializer::read(inputName);
        if ( mgfModel != nullptr ) {
            SceneBuilder::sceneBuilderApplyModelToMgfContext(mgfContext, mgfModel);
        }
    } else {
#ifdef MGF_ENABLED
        mgfModel = MgfParserLoader::readMgf(fileName, mgfContext);
        if ( mgfModel != nullptr
             && batchOptions != nullptr
             && batchOptions->exportBinary
             && batchOptions->binaryOutputFilename != nullptr
             && batchOptions->binaryOutputFilename[0] != '\0' ) {
            java::System::err.printf(
                "Exporting loaded ParseSnapshotContext to binary '%s' ... ",
                batchOptions->binaryOutputFilename);
            java::System::err.flush();
            const bool binarySaved = BinaryModelSerializer::write(
                mgfModel,
                batchOptions->binaryOutputFilename);
            if ( binarySaved ) {
                java::System::err.printf("done.\n");
            } else {
                java::System::err.printf("failed.\n");
                Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", java::String::format("Could not export ParseSnapshotContext binary to '%s'", batchOptions->binaryOutputFilename));
            }
        }
#else
        java::System::err.printf(
            "ERROR: MGF input requires MGF support. Rebuild with CMake flag '-DWITH_MGF=ON'.\n");
        java::System::err.flush();
        Logger::reportMessage("SceneBuilder", Logger::ERROR, "SceneBuilder::sceneBuilderReadFile", "Only '.bin' input files are supported in this build.");
#endif
    }

    scene->geometryList = mgfModel == nullptr ? nullptr : mgfModel->geometries;
    SceneBuilder::sceneBuilderFillFacesBackPointers(scene->geometryList);

    long long t = java::System::nanoTime();
    java::System::err.printf(
        "Reading took %g secs.\n",
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    delete[] currentDirectory;
    delete[] fallbackBinaryInputName;

    // Check for errors
    if ( scene->geometryList == nullptr || scene->geometryList->size() == 0 ) {
        return false; // Not successful
    }

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    java::System::err.printf("Building patch list ... ");
    java::System::err.flush();

    scene->patchList = new java::ArrayList<Patch *>();
    SceneBuilder::sceneBuilderPatchList(scene->geometryList, scene->patchList);

    t = java::System::nanoTime();
    java::System::err.printf(
        "%g secs.\n",
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Build the list of patches on light sources from the patch list
    java::System::err.printf("Building light source patch list ... ");
    java::System::err.flush();

    SceneBuilder::sceneBuilderFillLightSourcePatchList(scene);

    t = java::System::nanoTime();
    java::System::err.printf(
        "%g secs.\n",
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Build a cluster hierarchy for the new scene
    java::System::err.printf("Building cluster hierarchy ... ");
    java::System::err.flush();

    scene->clusteredRootGeometry = SceneBuilder::sceneBuilderCreateClusterHierarchy(scene->patchList);

    // The top-level clusters, as RenderPark's ClusteredWorld: the children of
    // the root cluster, or the root itself when it is not a compound
    if ( scene->clusteredRootGeometry->className == GeometryClassId::COMPOUND ) {
        const java::ArrayList<RpkGeometry *> *children = static_cast<const Compound *>(scene->clusteredRootGeometry)->children;
        for ( int i = 0; children != nullptr && i < children->size(); i++ ) {
            scene->clusteredGeometryList->add(children->get(i));
        }
    } else {
        scene->clusteredGeometryList->add(scene->clusteredRootGeometry);
        Logger::reportMessage("SceneBuilder", Logger::WARNING, "", "Strange clusters for this world ...");
    }

    // Dense face indices in depth-first order of the cluster hierarchy
    scene->buildFaceTable();

    t = java::System::nanoTime();
    java::System::err.printf(
        "%g secs.\n",
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Create the scene query, over the index the -scene-acceleration option chose
    scene->buildSceneQuery();

    t = java::System::nanoTime();
    java::System::err.printf(
        "Scene index (%s) creation took %g secs.\n",
        SceneQuery::methodName(scene->getAccelerationMethod()),
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    java::System::err.printf("Computing some scene statistics ... ");
    java::System::err.flush();

    Statistics::instance().reader.numberOfPatches = Statistics::instance().reader.numberOfElements;
    SceneBuilder::sceneBuilderComputeStats(scene);
    const Cie &colorSpace = toneMapOptions.getColorSpace();
    Statistics::instance().radiance.referenceLuminance = 5.42 * ((1.0 - colorSpace.spectrumGray(Statistics::instance().radiance.averageReflectivity.getR(), Statistics::instance().radiance.averageReflectivity.getG(), Statistics::instance().radiance.averageReflectivity.getB())) *
                                                   colorSpace.spectrumLuminance(Statistics::instance().radiance.estimatedAverageRadiance.getR(), Statistics::instance().radiance.estimatedAverageRadiance.getG(), Statistics::instance().radiance.estimatedAverageRadiance.getB()));

    t = java::System::nanoTime();
    java::System::err.printf(
        "%g secs.\n",
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Initialize tone mapping
    java::System::err.printf("Initializing tone mapping ... ");
    java::System::err.flush();

    Adaptation::initSceneAdaptation(scene->patchList, toneMapOptions);

    t = java::System::nanoTime();
    java::System::err.printf(
        "%g secs.\n",
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Print statistics report
    java::System::out.printf("\nStats: radiance.totalEmittedPower ................: %f W\n"
           "         radiance.estimatedAverageRadiance .........: %f W / sr\n"
           "         averageReflectivity ..............: %f\n"
           "         radiance.maxSelfEmittedRadiance ...........: %f W / sr\n"
           "         radiance.maxSelfEmittedPower ..............: %f W\n"
           "         toneMapOptions.realWorldAdaptionLuminance .........: %f cd / m2\n"
           "         totalArea ........................: %f m2\n",
           colorSpace.spectrumGray(Statistics::instance().radiance.totalEmittedPower.getR(), Statistics::instance().radiance.totalEmittedPower.getG(), Statistics::instance().radiance.totalEmittedPower.getB()),
           colorSpace.spectrumGray(Statistics::instance().radiance.estimatedAverageRadiance.getR(), Statistics::instance().radiance.estimatedAverageRadiance.getG(), Statistics::instance().radiance.estimatedAverageRadiance.getB()),
           colorSpace.spectrumGray(Statistics::instance().radiance.averageReflectivity.getR(), Statistics::instance().radiance.averageReflectivity.getG(), Statistics::instance().radiance.averageReflectivity.getB()),
           colorSpace.spectrumGray(Statistics::instance().radiance.maxSelfEmittedRadiance.getR(), Statistics::instance().radiance.maxSelfEmittedRadiance.getG(), Statistics::instance().radiance.maxSelfEmittedRadiance.getB()),
           colorSpace.spectrumGray(Statistics::instance().radiance.maxSelfEmittedPower.getR(), Statistics::instance().radiance.maxSelfEmittedPower.getG(), Statistics::instance().radiance.maxSelfEmittedPower.getB()),
           toneMapOptions.realWorldAdaptionLuminance,
           Statistics::instance().radiance.totalArea);
    //scene->print();

    // Initialize radiance for the freshly loaded scene
    java::System::err.printf("Initializing radiance method ... ");
    java::System::err.flush();

    Radiance::setRadianceMethod(mgfContext->radianceMethod, scene, &toneMapOptions);

    t = java::System::nanoTime();
    java::System::err.printf(
        "%g secs.\n",
        static_cast<double>(static_cast<double>(t - last) / 1000000000.0));

    // Remove possible render hooks
    RenderHookList::removeAllRenderHooks();

    SceneBuilder::removeEmptyMeshSurfaces(mgfContext, scene->geometryList);

    java::System::err.printf("Initialisations done.\n");

    return true;
}

void
SceneBuilder::sceneBuilderCreateModel(
    const int *argc,
    char *const *argv,
    ParseRuntimeContext *mgfContext,
    Scene *scene,
    ToneMappingContext &toneMapOptions)
{
    const BatchOptions *batchOptions = Batch::batchGetOptions();
    if ( batchOptions != nullptr
         && batchOptions->importBinary
         && batchOptions->binaryInputFilename != nullptr
         && batchOptions->binaryInputFilename[0] != '\0' ) {
        if ( !SceneBuilder::sceneBuilderReadFile(batchOptions->binaryInputFilename, mgfContext, scene, toneMapOptions) ) {
            java::System::exit(1);
        }
        return;
    }

    // All options should have disappeared from argv now
    if ( *argc > 1 ) {
        if ( *argv[1] == '-' ) {
            Logger::reportMessage("SceneBuilder", Logger::ERROR, "", java::String::format("Unrecognized option '%s'", argv[1]));
        } else if ( !SceneBuilder::sceneBuilderReadFile(argv[1], mgfContext, scene, toneMapOptions) ) {
            java::System::exit(1);
        }
    }
}
