#include <string.h>

#include "java/io/FileInputStream.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/Error.h"
#include "common/statistics/Statistics.h"
#include "scene/PatchClusterOctreeNode.h"
#include "tonemap/ToneMap.h"
#include "numericalAnalysis/MeshSurfaceVisitor.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/bin/reader/BinaryModelDeserializer.h"
#include "io/bin/writer/BinaryModelSerializer.h"
#ifdef MGF_ENABLED
    #include "io/mgf/MgfParserLoader.h"
#endif
#include "render/RenderHookList.h"
#include "render/ScreenBuffer.h"
#include "app/Adaptation.h"
#include "app/Batch.h"
#include "app/options/OptionsGroupCore.h"
#include "app/Radiance.h"
#include "app/SceneBuilder.h"

void
SceneBuilder::sceneBldPtchAccumStats(Patch *patch) {
    ColorRgb emittance = PatchVisitor::averageEmittance(patch, XxdfComponentFlagInfo::ALL_COMPONENTS);
    ColorRgb reflectance = PatchVisitor::averageNormalAlbedo(patch, BsdfComponentInfo::BSDF_ALL_COMPONENTS);
    ColorRgb power;

    Statistics::instance().radiance.totalArea += patch->area;
    power.scaledCopy(patch->area, emittance);
    Statistics::instance().radiance.totalEmittedPower.add(Statistics::instance().radiance.totalEmittedPower, power);
    Statistics::instance().radiance.averageReflectivity.addScaled(Statistics::instance().radiance.averageReflectivity, patch->area, reflectance);
    emittance.scale(1.0f / ((float)(M_PI)));
    Statistics::instance().radiance.maxSelfEmittedRadiance.maximum(emittance, Statistics::instance().radiance.maxSelfEmittedRadiance);
    Statistics::instance().radiance.maxSelfEmittedPower.maximum(power, Statistics::instance().radiance.maxSelfEmittedPower);
}

void
SceneBuilder::sceneBuilderComputeStats(Scene *scene) {
    Vector3D zero;
    ColorRgb one;
    ColorRgb averageAbsorption;
    ColorRgb BP;

    one.setMonochrome(1.0f);
    zero.set(0, 0, 0);

    // Initialize
    Statistics::instance().radiance.totalEmittedPower.clear();
    Statistics::instance().radiance.averageReflectivity.clear();
    Statistics::instance().radiance.maxSelfEmittedRadiance.clear();
    Statistics::instance().radiance.maxSelfEmittedPower.clear();
    Statistics::instance().radiance.totalArea = 0.0;

    // Accumulate
    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        SceneBuilder::sceneBldPtchAccumStats(scene->patchList->get(i));
    }

    // Averages
    Statistics::instance().radiance.averageReflectivity.scaleInverse(
            Statistics::instance().radiance.totalArea,
            Statistics::instance().radiance.averageReflectivity);
    averageAbsorption.subtract(one, Statistics::instance().radiance.averageReflectivity);
    Statistics::instance().radiance.estimatedAverageRadiance.scaleInverse(
            ((float)(M_PI)) * Statistics::instance().radiance.totalArea,
            Statistics::instance().radiance.totalEmittedPower);

    // Include background radiation
    BP = Background::backgroundPower(scene->background, &zero);
    BP.scale(1.0f / (4.0f * ((float)(M_PI))));
    Statistics::instance().radiance.totalEmittedPower.add(Statistics::instance().radiance.totalEmittedPower, BP);
    Statistics::instance().radiance.estimatedAverageRadiance.add(Statistics::instance().radiance.estimatedAverageRadiance, BP);
    Statistics::instance().radiance.estimatedAverageRadiance.divide(Statistics::instance().radiance.estimatedAverageRadiance, averageAbsorption);

    Statistics::instance().potential.totalDirectPotential = 0.0;
    Statistics::instance().potential.maxDirectPotential = 0.0;
    Statistics::instance().potential.averageDirectPotential = 0.0;
    Statistics::instance().potential.maxDirectImportance = 0.0;
}

/**
Adds the background to the global light source patch list
*/
void
SceneBuilder::sceneBldAddBgTLightSrcList(Scene *scene) {
    if ( scene->background != NULL && scene->background->bkgPatch != NULL ) {
        scene->lightSourcePatchList->add(scene->background->bkgPatch);
        Statistics::instance().reader.numberOfLightSources++;
    }
}

/**
Adds the patch to the global light source patch list if the patch is on
a light source (i.e. when the surfaces material has a non-null edf)
*/
void
SceneBuilder::sceBldAddPtcTLigSrcLisIfLigSrc(ArrayList<Patch *> *lights, Patch *patch) {
    if ( patch != NULL
         && patch->material != NULL
         && patch->material->getEdf() != NULL ) {
        lights->add(patch);
        Statistics::instance().reader.numberOfLightSources++;
    }
}

/**
Build the global light source patch list
*/
void
SceneBuilder::sceneBldFillLightSrcPtchList(Scene *scene) {
    ArrayList<Patch *> *lights = new ArrayList<Patch *>();
    Statistics::instance().reader.numberOfLightSources = 0;

    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        SceneBuilder::sceBldAddPtcTLigSrcLisIfLigSrc(lights, scene->patchList->get(i));
    }

    SceneBuilder::sceneBldAddBgTLightSrcList(scene);
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
Geometry *
SceneBuilder::sceneBldCreateClustHier(const ArrayList<Patch *> *patches) {
    PatchClusterOctreeNode *rootCluster;
    Geometry *rootGeometry;

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
SceneBuilder::sceneBuilderPatchList(const ArrayList<Geometry *> *geometryList, ArrayList<Patch *> *patchList) {
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        if ( geometry->isCompound() ) {
            // Recursive case
            const Compound *compound = ((const Compound *)(geometry));
            SceneBuilder::sceneBuilderPatchList(compound->children, patchList);
        } else {
            // Trivial case
            const ArrayList<Patch *> *patchesFromNonCompounds = Geometry::patchListReference(geometry);

            for ( int j = 0; patchesFromNonCompounds != NULL && j < patchesFromNonCompounds->size(); j++ ) {
                Patch *patch = patchesFromNonCompounds->get(j);
                if ( patch != NULL ) {
                    patchList->add(patch);
                }
            }
        }
    }
}

void
SceneBuilder::sceneBldFillFcsBackPntrs(const ArrayList<Geometry *> *geometryList) {
    if ( geometryList == NULL ) {
        return;
    }
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        if ( geometry == NULL ) {
            continue;
        }
        if ( geometry->isCompound() ) {
            const Compound *compound = ((const Compound *)(geometry));
            SceneBuilder::sceneBldFillFcsBackPntrs(compound->children);
            continue;
        }
        if ( geometry->className == SURFACE_MESH ) {
            MeshSurfaceVisitor::fillFacesBackPointers(((MeshSurface *)(geometry)));
        }
    }
}

void
SceneBuilder::sceneBldCollectGeomsRec(
    const ArrayList<Geometry *> *source,
    ArrayList<Geometry *> *target)
{
    if ( source == NULL || target == NULL ) {
        return;
    }

    for ( int i = 0; i < source->size(); i++ ) {
        Geometry *geometry = source->get(i);
        if ( geometry == NULL ) {
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
            const Compound *compound = ((const Compound *)(geometry));
            SceneBuilder::sceneBldCollectGeomsRec(compound->children, target);
        }
    }
}

void
SceneBuilder::sceneBldApplyMdlTMgfCtx(ParseRuntimeContext *mgfContext, ParseSnapshotContext *mgfModel) {
    if ( mgfContext == NULL || mgfModel == NULL ) {
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

    mgfContext->currentMaterial = NULL;
    if ( mgfContext->materials != NULL && mgfContext->currentMaterialName != NULL ) {
        for ( int i = 0; i < mgfContext->materials->size(); i++ ) {
            Material *material = mgfContext->materials->get(i);
            if ( material != NULL
                 && material->getName() != NULL
                 && strcmp(material->getName(), mgfContext->currentMaterialName) == 0 ) {
                mgfContext->currentMaterial = material;
                break;
            }
        }
    }

    if ( mgfContext->allGeometries != NULL ) {
        mgfContext->allGeometries->dispose();
        delete mgfContext->allGeometries;
    }
    mgfContext->allGeometries = new ArrayList<Geometry *>();
    SceneBuilder::sceneBldCollectGeomsRec(mgfModel->currentGeometryList, mgfContext->allGeometries);
    if ( mgfModel->geometries != mgfModel->currentGeometryList ) {
        SceneBuilder::sceneBldCollectGeomsRec(mgfModel->geometries, mgfContext->allGeometries);
    }
}

void
SceneBuilder::removeEmptyMeshSurfaces(ParseRuntimeContext *mgfContext, ArrayList<Geometry *> *geometryList) {
    for ( int i = 0; i < geometryList->size(); i++ ) {
        const Geometry *geometry = geometryList->get(i);
        if ( geometry->className == SURFACE_MESH ) {
            const MeshSurface *mesh = ((const MeshSurface *)(geometry));
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

bool
SceneBuilder::sceneBuilderHasExtension(const char *fileName, const char *extension) {
    if ( fileName == NULL || extension == NULL ) {
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
SceneBuilder::sceneBldBldBnryFllbcPath(const char *mgfFileName) {
    if ( !sceneBuilderHasExtension(mgfFileName, ".mgf") ) {
        return NULL;
    }

    const size_t length = strlen(mgfFileName);
    char *fallbackFileName = new char[length + 1];
    memcpy(fallbackFileName, mgfFileName, length + 1);
    memcpy(fallbackFileName + length - 4, ".bin", 5);

    return fallbackFileName;
}

bool
SceneBuilder::sceneBldIReadRegFile(const char *fileName) {
    if ( fileName == NULL || fileName[0] == '\0' ) {
        return false;
    }

    File file(fileName);
    if ( !file.exists() || !file.isFile() || !file.canRead() ) {
        return false;
    }

    FileInputStream input(fileName);
    const int firstByte = input.read();
    input.close();
    return firstByte >= 0;
}

bool
SceneBuilder::sceneBldValReadFile(
    const char *fileName,
    const char *fileRole)
{
    File file(fileName);
    if ( !file.exists() ) {
        Error::error(
            "SceneBuilder::sceneBuilderReadFile",
            "Requested %s file '%s' does not exist",
            fileRole,
            fileName);
        return false;
    }
    if ( !file.isFile() ) {
        Error::error(
            "SceneBuilder::sceneBuilderReadFile",
            "Requested %s file '%s' is not a regular file",
            fileRole,
            fileName);
        return false;
    }
    if ( !file.canRead() ) {
        Error::error(
            "SceneBuilder::sceneBuilderReadFile",
            "Requested %s file '%s' is not readable",
            fileRole,
            fileName);
        return false;
    }

    FileInputStream input(fileName);
    const int firstByte = input.read();
    input.close();

    if ( firstByte < 0 ) {
        Error::error(
            "SceneBuilder::sceneBuilderReadFile",
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
bool
SceneBuilder::sceneBuilderReadFile(
    const char *fileName,
    ParseRuntimeContext *mgfContext,
    Scene *scene,
    ToneMappingContext &toneMapOptions)
{
    const BatchOptions *batchOptions = Batch::batchGetOptions();
    const bool importBinaryOption =
        batchOptions != NULL
        && batchOptions->importBinary
        && batchOptions->binaryInputFilename != NULL
        && batchOptions->binaryInputFilename[0] != '\0';
    const char *requestedInputName = importBinaryOption ? batchOptions->binaryInputFilename : fileName;

    bool readBinaryModel =
        importBinaryOption
        || SceneBuilder::sceneBuilderHasExtension(requestedInputName, ".bin");

    char *fallbackBinaryInputName = NULL;
    const char *inputName = requestedInputName;

#ifdef MGF_ENABLED
    if ( !readBinaryModel
         && SceneBuilder::sceneBuilderHasExtension(requestedInputName, ".mgf")
         && !SceneBuilder::sceneBldIReadRegFile(requestedInputName) ) {
        fallbackBinaryInputName = SceneBuilder::sceneBldBldBnryFllbcPath(requestedInputName);
        if ( fallbackBinaryInputName != NULL
             && SceneBuilder::sceneBldIReadRegFile(fallbackBinaryInputName) ) {
            readBinaryModel = true;
            inputName = fallbackBinaryInputName;
        }
    }
#else
    if ( !readBinaryModel ) {
        if ( SceneBuilder::sceneBuilderHasExtension(requestedInputName, ".mgf") ) {
            fallbackBinaryInputName = SceneBuilder::sceneBldBldBnryFllbcPath(requestedInputName);
            if ( fallbackBinaryInputName != NULL
                 && SceneBuilder::sceneBldIReadRegFile(fallbackBinaryInputName) ) {
                readBinaryModel = true;
                inputName = fallbackBinaryInputName;
            } else {
                System::err.printf("MGF_ENABLED was OFF at compile time.\n");
                System::err.flush();
                Error::error(
                    "SceneBuilder::sceneBuilderReadFile",
                    "MGF_ENABLED was OFF at compile time. Requested MGF input '%s' could not be loaded and fallback binary '%s' is not available.",
                    requestedInputName,
                    fallbackBinaryInputName != NULL ? fallbackBinaryInputName : "(not derivable)");
                delete[] fallbackBinaryInputName;
                return false;
            }
        } else {
            System::err.printf("MGF_ENABLED was OFF at compile time.\n");
            System::err.flush();
            Error::error(
                "SceneBuilder::sceneBuilderReadFile",
                "MGF_ENABLED was OFF at compile time. Only '.bin' input files are supported.");
            return false;
        }
    }
#endif

    // Check whether the file can be opened/read
    if ( readBinaryModel && !SceneBuilder::sceneBldValReadFile(inputName, "binary model") ) {
        delete[] fallbackBinaryInputName;
        return false;
    }

    if ( !readBinaryModel && fileName[0] != '#' ) {
        if ( !SceneBuilder::sceneBldValReadFile(fileName, "scene") ) {
            delete[] fallbackBinaryInputName;
            return false;
        }
    }

    // Get current directory from the filename
    unsigned long n = strlen(inputName) + 1;

    char *currentDirectory = new char[n];
    Formatter::format(currentDirectory, ((int)(n)), "%s", inputName);
    char *slash = strrchr(currentDirectory, '/');
    if ( slash != NULL ) {
        *slash = '\0';
    } else {
        *currentDirectory = '\0';
    }

    // Prepare if errors occur when reading the new scene will abort
    scene->geometryList = NULL;

    Patch::setNextId(1);
    if ( scene->background != NULL ) {
        delete scene->background;
    }
    scene->background = OptionsGroupCore::createBackground();

    // Read the source scene description into a ParseSnapshotContext snapshot
    System::err.printf("Reading the scene from file '%s' ... \n", inputName);
    long last = System::nanoTime();
    ParseSnapshotContext *mgfModel = NULL;

    if ( readBinaryModel ) {
        mgfModel = BinaryModelDeserializer::read(inputName);
        if ( mgfModel != NULL ) {
            SceneBuilder::sceneBldApplyMdlTMgfCtx(mgfContext, mgfModel);
        }
    } else {
#ifdef MGF_ENABLED
        mgfModel = MgfParserLoader::readMgf(fileName, mgfContext);
        if ( mgfModel != NULL
             && batchOptions != NULL
             && batchOptions->exportBinary
             && batchOptions->binaryOutputFilename != NULL
             && batchOptions->binaryOutputFilename[0] != '\0' ) {
            System::err.printf(
                "Exporting loaded ParseSnapshotContext to binary '%s' ... ",
                batchOptions->binaryOutputFilename);
            System::err.flush();
            const bool binarySaved = BinaryModelSerializer::write(
                mgfModel,
                batchOptions->binaryOutputFilename);
            if ( binarySaved ) {
                System::err.printf("done.\n");
            } else {
                System::err.printf("failed.\n");
                Error::error(
                    "SceneBuilder::sceneBuilderReadFile",
                    "Could not export ParseSnapshotContext binary to '%s'",
                    batchOptions->binaryOutputFilename);
            }
        }
#else
        System::err.printf("MGF_ENABLED was OFF at compile time.\n");
        System::err.flush();
        Error::error(
            "SceneBuilder::sceneBuilderReadFile",
            "MGF_ENABLED was OFF at compile time. Only '.bin' input files are supported.");
#endif
    }

    scene->geometryList = mgfModel == NULL ? NULL : mgfModel->geometries;
    SceneBuilder::sceneBldFillFcsBackPntrs(scene->geometryList);

    long t = System::nanoTime();
    System::err.printf(
        "Reading took %g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));
    last = t;

    delete[] currentDirectory;
    delete[] fallbackBinaryInputName;

    // Check for errors
    if ( scene->geometryList == NULL || scene->geometryList->size() == 0 ) {
        return false; // Not successful
    }

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    System::err.printf("Building patch list ... ");
    System::err.flush();

    scene->patchList = new ArrayList<Patch *>();
    SceneBuilder::sceneBuilderPatchList(scene->geometryList, scene->patchList);

    t = System::nanoTime();
    System::err.printf(
        "%g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));
    last = t;

    // Build the list of patches on light sources from the patch list
    System::err.printf("Building light source patch list ... ");
    System::err.flush();

    SceneBuilder::sceneBldFillLightSrcPtchList(scene);

    t = System::nanoTime();
    System::err.printf(
        "%g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));
    last = t;

    // Build a cluster hierarchy for the new scene
    System::err.printf("Building cluster hierarchy ... ");
    System::err.flush();

    scene->clusteredRootGeometry = SceneBuilder::sceneBldCreateClustHier(scene->patchList);

    if ( scene->clusteredRootGeometry->className != COMPOUND ) {
        Error::warning(NULL, "Strange clusters for this world ...");
    }

    t = System::nanoTime();
    System::err.printf(
        "%g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));
    last = t;

    // Create the scene level voxel grid
    scene->voxelGrid = new VoxelGrid(scene->clusteredRootGeometry);

    t = System::nanoTime();
    System::err.printf(
        "Voxel grid creation took %g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    System::err.printf("Computing some scene statistics ... ");
    System::err.flush();

    Statistics::instance().reader.numberOfPatches = Statistics::instance().reader.numberOfElements;
    SceneBuilder::sceneBuilderComputeStats(scene);
    Statistics::instance().radiance.referenceLuminance = 5.42 * ((1.0 - Statistics::instance().radiance.averageReflectivity.gray()) *
                                                   Statistics::instance().radiance.estimatedAverageRadiance.luminance());

    t = System::nanoTime();
    System::err.printf(
        "%g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));
    last = t;

    // Initialize tone mapping
    System::err.printf("Initializing tone mapping ... ");
    System::err.flush();

    Adaptation::initSceneAdaptation(scene->patchList, toneMapOptions);

    t = System::nanoTime();
    System::err.printf(
        "%g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));
    last = t;

    // Print statistics report
    System::out.printf("\nStats: radiance.totalEmittedPower ................: %f W\n"
           "         radiance.estimatedAverageRadiance .........: %f W / sr\n"
           "         averageReflectivity ..............: %f\n"
           "         radiance.maxSelfEmittedRadiance ...........: %f W / sr\n"
           "         radiance.maxSelfEmittedPower ..............: %f W\n"
           "         toneMapOptions.realWorldAdaptionLuminance .........: %f cd / m2\n"
           "         totalArea ........................: %f m2\n",
           Statistics::instance().radiance.totalEmittedPower.gray(),
           Statistics::instance().radiance.estimatedAverageRadiance.gray(),
           Statistics::instance().radiance.averageReflectivity.gray(),
           Statistics::instance().radiance.maxSelfEmittedRadiance.gray(),
           Statistics::instance().radiance.maxSelfEmittedPower.gray(),
           toneMapOptions.realWorldAdaptionLuminance,
           Statistics::instance().radiance.totalArea);
    //scene->print();

    // Initialize radiance for the freshly loaded scene
    System::err.printf("Initializing radiance method ... ");
    System::err.flush();

    Radiance::setRadianceMethod(mgfContext->radianceMethod, scene, &toneMapOptions);

    t = System::nanoTime();
    System::err.printf(
        "%g secs.\n",
        ((float)(((double)(t - last)) / 1000000000.0)));

    // Remove possible render hooks
    RenderHookList::removeAllRenderHooks();

    SceneBuilder::removeEmptyMeshSurfaces(mgfContext, scene->geometryList);

    System::err.printf("Initialisations done.\n");

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
    if ( batchOptions != NULL
         && batchOptions->importBinary
         && batchOptions->binaryInputFilename != NULL
         && batchOptions->binaryInputFilename[0] != '\0' ) {
        if ( !SceneBuilder::sceneBuilderReadFile(batchOptions->binaryInputFilename, mgfContext, scene, toneMapOptions) ) {
            System::exit(1);
        }
        return;
    }

    // All options should have disappeared from argv now
    if ( *argc > 1 ) {
        if ( *argv[1] == '-' ) {
            Error::error(NULL, "Unrecognized option '%s'", argv[1]);
        } else if ( !SceneBuilder::sceneBuilderReadFile(argv[1], mgfContext, scene, toneMapOptions) ) {
            System::exit(1);
        }
    }
}
