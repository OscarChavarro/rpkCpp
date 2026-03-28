#include <cstring>

#include "java/io/FileInputStream.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"

#include "common/Error.h"
#include "common/Statistics.h"

#include "scene/PatchClusterOctreeNode.h"

#include "tonemap/ToneMap.h"

#include "numericalAnalysis/MeshSurfaceVisitor.h"
#include "numericalAnalysis/PatchVisitor.h"

#include "io/bin/BinaryModelReader.h"
#include "io/bin/BinaryModelWriter.h"

#include "io/mgf/readmgf.h"

#include "render/renderhook.h"
#include "render/ScreenBuffer.h"

#include "app/adaptation.h"
#include "app/batch.h"
#include "app/commandLine.h"
#include "app/radiance.h"
#include "app/sceneBuilder.h"

static void
sceneBuilderPatchAccumulateStats(Patch *patch) {
    ColorRgb E = PatchVisitor::averageEmittance(patch, ALL_COMPONENTS);
    ColorRgb R = PatchVisitor::averageNormalAlbedo(patch, BSDF_ALL_COMPONENTS);
    ColorRgb power;

    GLOBAL_statistics.totalArea += patch->area;
    power.scaledCopy(patch->area, E);
    GLOBAL_statistics.totalEmittedPower.add(GLOBAL_statistics.totalEmittedPower, power);
    GLOBAL_statistics.averageReflectivity.addScaled(GLOBAL_statistics.averageReflectivity, patch->area, R);
    E.scale(1.0f / static_cast<float>(M_PI));
    GLOBAL_statistics.maxSelfEmittedRadiance.maximum(E, GLOBAL_statistics.maxSelfEmittedRadiance);
    GLOBAL_statistics.maxSelfEmittedPower.maximum(power, GLOBAL_statistics.maxSelfEmittedPower);
}

static void
sceneBuilderComputeStats(Scene *scene) {
    Vector3D zero;
    ColorRgb one;
    ColorRgb averageAbsorption;
    ColorRgb BP;

    one.setMonochrome(1.0f);
    zero.set(0, 0, 0);

    // Initialize
    GLOBAL_statistics.totalEmittedPower.clear();
    GLOBAL_statistics.averageReflectivity.clear();
    GLOBAL_statistics.maxSelfEmittedRadiance.clear();
    GLOBAL_statistics.maxSelfEmittedPower.clear();
    GLOBAL_statistics.totalArea = 0.0;

    // Accumulate
    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        sceneBuilderPatchAccumulateStats(scene->patchList->get(i));
    }

    // Averages
    GLOBAL_statistics.averageReflectivity.scaleInverse(
            GLOBAL_statistics.totalArea,
            GLOBAL_statistics.averageReflectivity);
    averageAbsorption.subtract(one, GLOBAL_statistics.averageReflectivity);
    GLOBAL_statistics.estimatedAverageRadiance.scaleInverse(
            static_cast<float>(M_PI) * GLOBAL_statistics.totalArea,
            GLOBAL_statistics.totalEmittedPower);

    // Include background radiation
    BP = backgroundPower(scene->background, &zero);
    BP.scale(1.0f / (4.0f * static_cast<float>(M_PI)));
    GLOBAL_statistics.totalEmittedPower.add(GLOBAL_statistics.totalEmittedPower, BP);
    GLOBAL_statistics.estimatedAverageRadiance.add(GLOBAL_statistics.estimatedAverageRadiance, BP);
    GLOBAL_statistics.estimatedAverageRadiance.divide(GLOBAL_statistics.estimatedAverageRadiance, averageAbsorption);

    GLOBAL_statistics.totalDirectPotential = 0.0;
    GLOBAL_statistics.maxDirectPotential = 0.0;
    GLOBAL_statistics.averageDirectPotential = 0.0;
    GLOBAL_statistics.maxDirectImportance = 0.0;
}

/**
Adds the background to the global light source patch list
*/
static void
sceneBuilderAddBackgroundToLightSourceList(Scene *scene) {
    if ( scene->background != nullptr && scene->background->bkgPatch != nullptr ) {
        scene->lightSourcePatchList->add(scene->background->bkgPatch);
        GLOBAL_statistics.numberOfLightSources++;
    }
}

/**
Adds the patch to the global light source patch list if the patch is on
a light source (i.e. when the surfaces material has a non-null edf)
*/
static void
sceneBuilderAddPatchToLightSourceListIfLightSource(java::ArrayList<Patch *> *lights, Patch *patch) {
    if ( patch != nullptr
         && patch->material != nullptr
         && patch->material->getEdf() != nullptr ) {
        lights->add(patch);
        GLOBAL_statistics.numberOfLightSources++;
    }
}

/**
Build the global light source patch list
*/
static void
sceneBuilderFillLightSourcePatchList(Scene *scene) {
    java::ArrayList<Patch *> *lights = new java::ArrayList<Patch *>();
    GLOBAL_statistics.numberOfLightSources = 0;

    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        sceneBuilderAddPatchToLightSourceListIfLightSource(lights, scene->patchList->get(i));
    }

    sceneBuilderAddBackgroundToLightSourceList(scene);
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
static Geometry *
sceneBuilderCreateClusterHierarchy(const java::ArrayList<Patch *> *patches) {
    PatchClusterOctreeNode *rootCluster;
    Geometry *rootGeometry;

    // Create a toplevel cluster containing (references to) all the patches in the scene
    rootCluster = new PatchClusterOctreeNode(patches);

    // Split the toplevel cluster recursively into sub-clusters
    rootCluster->splitCluster();
    //rootCluster->print(0);

    // Convert to a Geometry GLOBAL_stochasticRaytracing_hierarchy, disposing of the clusters
    rootGeometry = rootCluster->convertClusterToGeometry();

    delete rootCluster;
    return rootGeometry;
}

/**
Builds a linear list of patches making up all the geometries in the list, whether
they are primitive or not
*/
static void
sceneBuilderPatchList(const java::ArrayList<Geometry *> *geometryList, java::ArrayList<Patch *> *patchList) {
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        if ( geometry->isCompound() ) {
            // Recursive case
            const Compound *compound = static_cast<const Compound *>(geometry);
            sceneBuilderPatchList(compound->children, patchList);
        } else {
            // Trivial case
            const java::ArrayList<Patch *> *patchesFromNonCompounds = geomPatchArrayListReference(geometry);

            for ( int j = 0; patchesFromNonCompounds != nullptr && j < patchesFromNonCompounds->size(); j++ ) {
                Patch *patch = patchesFromNonCompounds->get(j);
                if ( patch != nullptr ) {
                    patchList->add(patch);
                }
            }
        }
    }
}

static void
sceneBuilderFillFacesBackPointers(const java::ArrayList<Geometry *> *geometryList) {
    if ( geometryList == nullptr ) {
        return;
    }
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        if ( geometry == nullptr ) {
            continue;
        }
        if ( geometry->isCompound() ) {
            const Compound *compound = static_cast<const Compound *>(geometry);
            sceneBuilderFillFacesBackPointers(compound->children);
            continue;
        }
        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            MeshSurfaceVisitor::fillFacesBackPointers(static_cast<MeshSurface *>(geometry));
        }
    }
}

static void
sceneBuilderCollectGeometriesRecursive(
    const java::ArrayList<Geometry *> *source,
    java::ArrayList<Geometry *> *target)
{
    if ( source == nullptr || target == nullptr ) {
        return;
    }

    for ( int i = 0; i < source->size(); i++ ) {
        Geometry *geometry = source->get(i);
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
            sceneBuilderCollectGeometriesRecursive(compound->children, target);
        }
    }
}

static void
sceneBuilderApplyModelToMgfContext(BaseContext *mgfContext, PersistedSceneModel *mgfModel) {
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
            Material *material = mgfContext->materials->get(i);
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
    mgfContext->allGeometries = new java::ArrayList<Geometry *>();
    sceneBuilderCollectGeometriesRecursive(mgfModel->currentGeometryList, mgfContext->allGeometries);
    if ( mgfModel->geometries != mgfModel->currentGeometryList ) {
        sceneBuilderCollectGeometriesRecursive(mgfModel->geometries, mgfContext->allGeometries);
    }
}

static void
removeEmptyMeshSurfaces(BaseContext *mgfContext, java::ArrayList<Geometry *> *geometryList) {
    for ( int i = 0; i < geometryList->size(); i++ ) {
        const Geometry *geometry = geometryList->get(i);
        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            const MeshSurface *mesh = static_cast<const MeshSurface *>(geometry);
            if ( mesh->vertices->size() == 0 ) {
                for ( int j = 0; j < mgfContext->allGeometries->size(); j++ ) {
                    const Geometry *deletionCandidate = mgfContext->allGeometries->get(j);
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

static bool
sceneBuilderValidateReadableFile(
    const char *fileName,
    const char *fileRole)
{
    java::io::File file(fileName);
    if ( !file.exists() ) {
        Error::error(
            "sceneBuilderReadFile",
            "Requested %s file '%s' does not exist",
            fileRole,
            fileName);
        return false;
    }
    if ( !file.isFile() ) {
        Error::error(
            "sceneBuilderReadFile",
            "Requested %s file '%s' is not a regular file",
            fileRole,
            fileName);
        return false;
    }
    if ( !file.canRead() ) {
        Error::error(
            "sceneBuilderReadFile",
            "Requested %s file '%s' is not readable",
            fileRole,
            fileName);
        return false;
    }

    java::io::FileInputStream input(fileName);
    const int firstByte = input.read();
    input.close();

    if ( firstByte < 0 ) {
        Error::error(
            "sceneBuilderReadFile",
            "Requested %s file '%s' is empty",
            fileRole,
            fileName);
        return false;
    }

    return true;
}

/**
Tries to read the scene in the given file. Returns false if not successful.
Returns true if successful
*/
static bool
sceneBuilderReadFile(const char *fileName, BaseContext *mgfContext, Scene *scene) {
    const BatchOptions *batchOptions = batchGetOptions();
    const bool importBinary =
        batchOptions != nullptr
        && batchOptions->importBinary
        && batchOptions->binaryInputFilename != nullptr
        && batchOptions->binaryInputFilename[0] != '\0';
    const char *inputName = importBinary ? batchOptions->binaryInputFilename : fileName;

    // Check whether the file can be opened/read
    if ( importBinary && !sceneBuilderValidateReadableFile(inputName, "binary model") ) {
        return false;
    }

    if ( !importBinary && fileName[0] != '#' ) {
        if ( !sceneBuilderValidateReadableFile(fileName, "scene") ) {
            return false;
        }
    }

    // Get current directory from the filename
    unsigned long n = strlen(inputName) + 1;

    char *currentDirectory = new char[n];
    java::util::Formatter::format(currentDirectory, static_cast<int>(n), "%s", inputName);
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
    scene->background = commandLineCreateBackground();

    // Read the source scene description into a PersistedSceneModel snapshot
    java::lang::System::err.printf("Reading the scene from file '%s' ... \n", inputName);
    long long last = java::lang::System::nanoTime();
    PersistedSceneModel *mgfModel = nullptr;

    if ( importBinary ) {
        mgfModel = BinaryModelReader::read(inputName);
        if ( mgfModel != nullptr ) {
            sceneBuilderApplyModelToMgfContext(mgfContext, mgfModel);
        }
    } else {
        mgfModel = readMgf(fileName, mgfContext);
        if ( mgfModel != nullptr
             && batchOptions != nullptr
             && batchOptions->exportBinary
             && batchOptions->binaryOutputFilename != nullptr
             && batchOptions->binaryOutputFilename[0] != '\0' ) {
            java::lang::System::err.printf(
                "Exporting loaded PersistedSceneModel to binary '%s' ... ",
                batchOptions->binaryOutputFilename);
            java::lang::System::err.flush();
            const bool binarySaved = BinaryModelWriter::write(
                mgfModel,
                batchOptions->binaryOutputFilename);
            if ( binarySaved ) {
                java::lang::System::err.printf("done.\n");
            } else {
                java::lang::System::err.printf("failed.\n");
                Error::error(
                    "sceneBuilderReadFile",
                    "Could not export PersistedSceneModel binary to '%s'",
                    batchOptions->binaryOutputFilename);
            }
        }
    }

    scene->geometryList = mgfModel == nullptr ? nullptr : mgfModel->geometries;
    sceneBuilderFillFacesBackPointers(scene->geometryList);

    long long t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "Reading took %g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    delete[] currentDirectory;

    // Check for errors
    if ( scene->geometryList == nullptr || scene->geometryList->size() == 0 ) {
        return false; // Not successful
    }

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    java::lang::System::err.printf("Building patch list ... ");
    java::lang::System::err.flush();

    scene->patchList = new java::ArrayList<Patch *>();
    sceneBuilderPatchList(scene->geometryList, scene->patchList);

    t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Build the list of patches on light sources from the patch list
    java::lang::System::err.printf("Building light source patch list ... ");
    java::lang::System::err.flush();

    sceneBuilderFillLightSourcePatchList(scene);

    t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Build a cluster hierarchy for the new scene
    java::lang::System::err.printf("Building cluster hierarchy ... ");
    java::lang::System::err.flush();

    scene->clusteredRootGeometry = sceneBuilderCreateClusterHierarchy(scene->patchList);

    if ( scene->clusteredRootGeometry->className != GeometryClassId::COMPOUND ) {
        Error::warning(nullptr, "Strange clusters for this world ...");
    }

    t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Create the scene level voxel grid
    scene->voxelGrid = new VoxelGrid(scene->clusteredRootGeometry);

    t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "Voxel grid creation took %g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    java::lang::System::err.printf("Computing some scene statistics ... ");
    java::lang::System::err.flush();

    GLOBAL_statistics.numberOfPatches = GLOBAL_statistics.numberOfElements;
    sceneBuilderComputeStats(scene);
    GLOBAL_statistics.referenceLuminance = 5.42 * ((1.0 - GLOBAL_statistics.averageReflectivity.gray()) *
                                                   GLOBAL_statistics.estimatedAverageRadiance.luminance());

    t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Initialize tone mapping
    java::lang::System::err.printf("Initializing tone mapping ... ");
    java::lang::System::err.flush();

    initSceneAdaptation(scene->patchList);

    t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));
    last = t;

    // Print statistics report
    java::lang::System::out.printf("\nStats: GLOBAL_statistics.totalEmittedPower ................: %f W\n"
           "         GLOBAL_statistics.estimatedAverageRadiance .........: %f W / sr\n"
           "         GLOBAL_statistics_averageReflectivity ..............: %f\n"
           "         GLOBAL_statistics.maxSelfEmittedRadiance ...........: %f W / sr\n"
           "         GLOBAL_statistics.maxSelfEmittedPower ..............: %f W\n"
           "         GLOBAL_toneMap_options.lwa (adaptationLuminance) ...: %f cd / m2\n"
           "         GLOBAL_statistics_totalArea ........................: %f m2\n",
           GLOBAL_statistics.totalEmittedPower.gray(),
           GLOBAL_statistics.estimatedAverageRadiance.gray(),
           GLOBAL_statistics.averageReflectivity.gray(),
           GLOBAL_statistics.maxSelfEmittedRadiance.gray(),
           GLOBAL_statistics.maxSelfEmittedPower.gray(),
           GLOBAL_toneMap_options.realWorldAdaptionLuminance,
           GLOBAL_statistics.totalArea);
    //scene->print();

    // Initialize radiance for the freshly loaded scene
    java::lang::System::err.printf("Initializing radiance method ... ");
    java::lang::System::err.flush();

    setRadianceMethod(mgfContext->radianceMethod, scene);

    t = java::lang::System::nanoTime();
    java::lang::System::err.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(t - last) / 1000000000.0));

    // Remove possible render hooks
    removeAllRenderHooks();

    removeEmptyMeshSurfaces(mgfContext, scene->geometryList);

    java::lang::System::err.printf("Initialisations done.\n");

    return true;
}

void
sceneBuilderCreateModel(
    const int *argc,
    char *const *argv,
    BaseContext *mgfContext,
    Scene *scene)
{
    const BatchOptions *batchOptions = batchGetOptions();
    if ( batchOptions != nullptr
         && batchOptions->importBinary
         && batchOptions->binaryInputFilename != nullptr
         && batchOptions->binaryInputFilename[0] != '\0' ) {
        if ( !sceneBuilderReadFile(batchOptions->binaryInputFilename, mgfContext, scene) ) {
            java::lang::System::exit(1);
        }
        return;
    }

    // All options should have disappeared from argv now
    if ( *argc > 1 ) {
        if ( *argv[1] == '-' ) {
            Error::error(nullptr, "Unrecognized option '%s'", argv[1]);
        } else if ( !sceneBuilderReadFile(argv[1], mgfContext, scene) ) {
            java::lang::System::exit(1);
        }
    }
}
