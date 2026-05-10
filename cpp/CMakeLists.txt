cmake_minimum_required(VERSION 3.10)
project(rpk)

option(WITH_RAYTRACING "Enable raytracing modules" ON)
option(WITH_OPENGL "Enable OpenGL modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

if(WITH_OPENGL)
    if(APPLE)
        find_library(OPENGL_FRAMEWORK OpenGL)
        find_library(GLUT_FRAMEWORK GLUT)
        if(NOT OPENGL_FRAMEWORK OR NOT GLUT_FRAMEWORK)
            message(WARNING "OpenGL support requested, but OpenGL/GLUT frameworks were not found. Building without OpenGL support.")
            set(WITH_OPENGL OFF)
        endif()
    else()
        find_library(RPK_OPENGL_GL_LIBRARY GL)
        find_library(RPK_OPENGL_GLU_LIBRARY GLU)
        find_library(RPK_OPENGL_GLUT_LIBRARY glut)
        if(NOT RPK_OPENGL_GL_LIBRARY OR NOT RPK_OPENGL_GLU_LIBRARY OR NOT RPK_OPENGL_GLUT_LIBRARY)
            message(WARNING "OpenGL support requested, but GL/GLU/GLUT libraries were not found. Building without OpenGL support.")
            set(WITH_OPENGL OFF)
        endif()
    endif()
endif()

add_compile_options(-ffast-math -O3)

include_directories(src)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MAIN_SRC
    src/java/lang/Object.cpp
    src/java/lang/Character.cpp
    src/java/lang/Float.cpp
    src/java/lang/String.cpp
    src/java/lang/System.cpp
    src/java/lang/ProcessBuilder.cpp
    src/java/lang/StringBuilder.cpp
    src/java/util/Formatter.cpp
    src/java/util/StringTokenizer.cpp
    src/java/io/InputStream.cpp
    src/java/io/OutputStream.cpp
    src/java/io/File.cpp
    src/java/io/FileInputStream.cpp
    src/java/io/FileOutputStream.cpp
    src/java/io/BufferedInputStream.cpp
    src/java/io/PrintStream.cpp
    src/common/logging/Logger.cpp
    src/common/memoryManagement/CppReAlloc.cpp
    src/common/color/ColorRgb.cpp
    src/common/color/Cie.cpp
    src/common/RenderOptions.cpp
    src/common/StratifiedSampling2D.cpp
    src/common/statistics/Statistics.cpp
    src/common/statistics/ReaderStatistics.cpp
    src/common/statistics/RadianceStatistics.cpp
    src/common/statistics/PotentialStatistics.cpp
    src/common/statistics/ShadowStatistics.cpp
    src/common/statistics/RayTracerStatistics.cpp
    src/common/linealAlgebra/Numeric.cpp
    src/common/linealAlgebra/Matrix2x2.cpp
    src/common/linealAlgebra/Vector3D.cpp
    src/common/linealAlgebra/Matrix4x4.cpp
    src/common/linealAlgebra/Matrix4x4d.cpp
    src/common/linealAlgebra/Vector3Dd.cpp
    src/common/linealAlgebra/CoordinateSystem.cpp
    src/common/linealAlgebra/Jacobian.cpp
    src/common/dataStructures/CircularListBase.cpp
    src/common/dataStructures/CircularListLink.cpp
    src/common/dataStructures/CircularListBaseIterator.cpp
    src/common/dataStructures/KDQuery.cpp
    src/common/dataStructures/KDTree.cpp
    src/material/RefractionIndex.cpp
    src/material/Texture.cpp
    src/material/Xxdf.cpp
    src/material/PhongEmittanceDistributionFunction.cpp
    src/material/PhongBidirectionalReflectanceDistributionFunction.cpp
    src/material/PhongBidirectionalTransmittanceDistributionFunction.cpp
    src/material/PhongBidirectionalScatteringDistributionFunction.cpp
    src/material/Material.cpp
    src/skin/PatchSet.cpp
    src/skin/RayHit.cpp
    src/skin/Compound.cpp
    src/skin/Geometry.cpp
    src/skin/Patch.cpp
    src/skin/BoundingBox.cpp
    src/skin/MinMaxBox.cpp
    src/skin/MeshSurface.cpp
    src/skin/Vertex.cpp
    src/skin/Element.cpp
    src/render/sgl/PolyClip.cpp
    src/render/sgl/PolyScanZ.cpp
    src/render/sgl/SglContext.cpp
    src/render/sgl/PolyScanFlat.cpp
    src/scene/Background.cpp
    src/scene/ConstantColorBackground.cpp
    src/scene/Plane.cpp
    src/scene/Camera.cpp
    src/scene/PatchClusterOctreeNode.cpp
    src/scene/VoxelData.cpp
    src/scene/VoxelGrid.cpp
    src/scene/RadianceMethod.cpp
    src/scene/Scene.cpp
    src/tonemap/ToneMappingContext.cpp
    src/tonemap/ToneMap.cpp
    src/tonemap/IdentityToneMap.cpp
    src/tonemap/LightnessToneMap.cpp
    src/tonemap/WardToneMap.cpp
    src/tonemap/TumblinRushmeierToneMap.cpp
    src/tonemap/RevisedTumblinRushmeierToneMap.cpp
    src/tonemap/FerwerdaToneMap.cpp
    src/numericalAnalysis/TriangleCubatureRule.cpp
    src/numericalAnalysis/QuadCubatureRule.cpp
    src/numericalAnalysis/quasiMonteCarlo/Halton.cpp
    src/numericalAnalysis/quasiMonteCarlo/ScrambledHalton.cpp
    src/numericalAnalysis/quasiMonteCarlo/Niederreiter31.cpp
    src/numericalAnalysis/quasiMonteCarlo/Faure.cpp
    src/numericalAnalysis/quasiMonteCarlo/Niederreiter63.cpp
    src/numericalAnalysis/quasiMonteCarlo/Sobol.cpp
    src/numericalAnalysis/PatchVisitor.cpp
    src/numericalAnalysis/MeshSurfaceVisitor.cpp
    src/io/wrapper/PersistenceElement.cpp
    src/io/wrapper/PipeInputStream.cpp
    src/io/wrapper/PipeOutputStream.cpp
    src/io/wrapper/FileUncompressWrapper.cpp
    src/io/wrapper/Vector3DPrinter.cpp
    src/io/wrapper/Matrix2x2Printer.cpp
    src/io/context/TransformContext.cpp
    src/io/context/EntityNamingContext.cpp
    src/io/context/ParseRuntimeContext.cpp
    src/io/context/ParseOptionsContext.cpp
    src/io/context/ReaderDispatchContext.cpp
    src/io/context/GeometryAssemblyContext.cpp
    src/io/context/MaterialSelectionContext.cpp
    src/io/context/ColorRegistryContext.cpp
    src/io/context/MaterialRegistryContext.cpp
    src/io/context/VertexRegistryContext.cpp
    src/io/context/ObjectScopeContext.cpp
    src/io/context/TransformScopeContext.cpp
    src/io/context/ColorContext.cpp
    src/io/context/TokenValidationContext.cpp
    src/io/context/ParseSnapshotContext.cpp
    src/io/bin/writer/BinaryModelSerializer.cpp
    src/io/bin/writer/BinaryModelSerializationGraph.cpp
    src/io/bin/reader/BinaryModelDeserializer.cpp
    src/io/bin/reader/BinaryModelReadPrimitives.cpp
    src/io/bin/reader/BinaryModelReadCleanup.cpp
    src/io/bin/reader/BinaryModelIndexListRef.cpp
    src/io/bin/reader/BinaryModelVertexRecordData.cpp
    src/io/bin/reader/BinaryModelPatchRecordData.cpp
    src/io/bin/reader/BinaryModelGeometryRecordData.cpp
    src/io/bin/reader/BinaryModelSnapshotRecordData.cpp
    src/io/wrl/VrmlWriter.cpp
    src/io/image/PicOutputHandle.cpp
    src/io/image/Dkcolor.cpp
    src/io/image/ImageOutputHandle.cpp
    src/io/image/PPMOutputHandle.cpp
    src/render/Potential.cpp
    src/render/SoftIdsWrapper.cpp
    src/render/Softids.cpp
    src/render/RadianceImageExporter.cpp
    src/render/RenderHookList.cpp
    src/render/Canvas.cpp
    src/render/ScreenBuffer.cpp
    src/galerkin/GalerkinState.cpp
    src/galerkin/GalerkinBasis.cpp
    src/galerkin/GalerkinRadianceMethod.cpp
    src/galerkin/Shaft.cpp
    src/galerkin/GalerkinElement.cpp
    src/galerkin/BasisTriGalerkin.cpp
    src/galerkin/Interaction.cpp
    src/galerkin/BasisQuadGalerkin.cpp
    src/galerkin/ShadowCache.cpp
    src/galerkin/processing/ScratchVisibilityStrategy.cpp
    src/galerkin/processing/ClusterTraversalStrategy.cpp
    src/galerkin/processing/ClusterCreationStrategy.cpp
    src/galerkin/processing/FormFactorStrategy.cpp
    src/galerkin/processing/FormFactorClusteredStrategy.cpp
    src/galerkin/processing/LinkingClusteredStrategy.cpp
    src/galerkin/processing/LinkingSimpleStrategy.cpp
    src/galerkin/processing/HierarchicalRefinementStrategy.cpp
    src/galerkin/processing/GatheringStrategy.cpp
    src/galerkin/processing/GatheringSimpleStrategy.cpp
    src/galerkin/processing/GatheringClusteredStrategy.cpp
    src/galerkin/processing/ShootingStrategy.cpp
    src/galerkin/processing/visitors/ScratchRendererVisitor.cpp
    src/galerkin/processing/visitors/MaximumRadianceVisitor.cpp
    src/galerkin/processing/visitors/PowerAccumulatorVisitor.cpp
    src/galerkin/processing/visitors/ProjectedAreaAccumulatorVisitor.cpp
    src/galerkin/processing/visitors/OrientedGathererVisitor.cpp
    src/galerkin/processing/visitors/DepthVisibilityGathererVisitor.cpp
    src/app/Adaptation.cpp
    src/app/options/OptionsGroupRadiance.cpp
    src/app/Radiance.cpp
    src/app/SceneBuilder.cpp
    src/app/Batch.cpp
    src/app/options/OptionsGroupCore.cpp
    src/app/options/OptionsGroupCamera.cpp
    src/app/options/OptionsGroupBatch.cpp
    src/app/options/OptionsGroupGalerkin.cpp
    src/app/options/OptionsGroupToneMapping.cpp
    src/app/options/OptionsGroupRender.cpp
    src/app/options/OptionsGroupRadianceMethod.cpp
    src/app/options/BatchOptions.cpp
    src/app/RpkApplication.cpp
    src/app/Main.cpp)

set(APP_RAYTRACING_SRC
    src/app/Raytrace.cpp
    src/app/options/OptionsGroupRaytracing.cpp
    src/app/options/OptionsGroupRaytracingMethod.cpp
    src/app/options/OptionsGroupStochasticRelaxationRadiosity.cpp
    src/app/options/OptionsGroupRandomWalkRadiosity.cpp
    src/app/options/OptionsGroupStochasticRaytracing.cpp
    src/app/options/OptionsGroupBidirectionalRaytracing.cpp
    src/app/options/OptionsGroupPhotonMap.cpp
    src/app/options/OptionsGroupRayMatter.cpp)

set(OPEN_GL_SRC
    src/render/opengl/Opengl.cpp
    src/render/opengl/RenderOpenGL.cpp
    src/render/opengl/GalerkinOpenGLRenderer.cpp
    src/render/opengl/visualDebugTools/GlutDebugState.cpp
    src/render/opengl/visualDebugTools/GlutHudConsole.cpp
    src/render/opengl/visualDebugTools/GlutDebugPatchHierarchy.cpp
    src/render/opengl/visualDebugTools/GlutDebugMode.cpp
    src/render/opengl/visualDebugTools/GlutDebugToolsModel.cpp
    src/render/opengl/visualDebugTools/GlutDebugToolsKeyControl.cpp
    src/render/opengl/visualDebugTools/PatchHitCandidate.cpp
    src/render/opengl/visualDebugTools/GlutDebugToolsMouseControl.cpp
    src/render/opengl/visualDebugTools/GlutDebugTools.cpp)

set(MGF_SRC
    src/io/mgf/MgfEntityControl.cpp
    src/io/mgf/MgfTessellationMath.cpp
    src/io/mgf/MgfSphereEntityExpander.cpp
    src/io/mgf/MgfTorusEntityExpander.cpp
    src/io/mgf/MgfCylinderEntityExpander.cpp
    src/io/mgf/MgfRingEntityTessellator.cpp
    src/io/mgf/MgfConeEntityTessellator.cpp
    src/io/mgf/MgfPrismEntityTessellator.cpp
    src/io/mgf/MgfFaceWithHolesEntityExpander.cpp
    src/io/mgf/MgfVertexFaceEntitySupport.cpp
    src/io/mgf/MgfObjectNameSupport.cpp
    src/io/mgf/MgfTransformationSupport.cpp
    src/io/mgf/MgfColorEntitySupport.cpp
    src/io/mgf/MgfMaterialEntitySupport.cpp
    src/io/mgf/MgfEntityHandlerAdapter.cpp
    src/io/mgf/MgfParserLoader.cpp)

set(RAYTRACING_SRC
    src/raycasting/common/RayTracer.cpp
    src/raycasting/common/SimpleRaytracingPathNode.cpp
    src/raycasting/common/Raytools.cpp
    src/raycasting/common/PixelFilter.cpp
    src/raycasting/common/BoxFilter.cpp
    src/raycasting/common/NormalFilter.cpp
    src/raycasting/common/TentFilter.cpp
    src/raycasting/simple/RayCaster.cpp
    src/raycasting/simple/RayMatter.cpp
    src/raycasting/raytracing/ScreenIterate.cpp
    src/raycasting/raytracing/ScreenIterateState.cpp
    src/raycasting/raytracing/SpecularSampler.cpp
    src/raycasting/raytracing/Sampler.cpp
    src/raycasting/raytracing/EyeSampler.cpp
    src/raycasting/raytracing/BsdfSampler.cpp
    src/raycasting/raytracing/SamplerConfig.cpp
    src/raycasting/raytracing/PixelSampler.cpp
    src/raycasting/bidirectionalRaytracing/DensityHitList.cpp
    src/raycasting/bidirectionalRaytracing/DensityBuffer.cpp
    src/raycasting/bidirectionalRaytracing/Kernel2D.cpp
    src/raycasting/bidirectionalRaytracing/BiPath.cpp
    src/raycasting/bidirectionalRaytracing/LightList.cpp
    src/raycasting/bidirectionalRaytracing/LightSampler.cpp
    src/raycasting/bidirectionalRaytracing/LightDirSampler.cpp
    src/raycasting/bidirectionalRaytracing/FlagChain.cpp
    src/raycasting/bidirectionalRaytracing/Spar.cpp
    src/raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.cpp
    src/raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer.cpp
    src/raycasting/photonMap/SampleGrid2D.cpp
    src/raycasting/photonMap/NormalQuery.cpp
    src/raycasting/photonMap/DiscreteSampling.cpp
    src/raycasting/photonMap/PhotonKDTree.cpp
    src/raycasting/photonMap/PhotonMapImportance.cpp
    src/raycasting/photonMap/ScreenSampler.cpp
    src/raycasting/photonMap/PhotonMapState.cpp
    src/raycasting/photonMap/Photon.cpp
    src/raycasting/photonMap/ImportanceMap.cpp
    src/raycasting/photonMap/PhotonMapRadianceMethod.cpp
    src/raycasting/photonMap/PhotonMap.cpp
    src/raycasting/photonMap/PhotonMapSampler.cpp
    src/raycasting/stochasticRaytracing/StochasticRaytracingState.cpp
    src/raycasting/stochasticRaytracing/StochasticRaytracer.cpp
    src/raycasting/stochasticRaytracing/StochasticRadiosityElement.cpp
    src/raycasting/stochasticRaytracing/Hierarchy.cpp
    src/raycasting/stochasticRaytracing/StochasticRaytracingPathNode.cpp
    src/raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.cpp
    src/raycasting/stochasticRaytracing/StochasticRadiosityElementRender.cpp
    src/raycasting/stochasticRaytracing/RandomWalkRadianceMethod.cpp
    src/raycasting/stochasticRaytracing/Coefficientsgalerkin.cpp
    src/raycasting/stochasticRaytracing/Basismcrad.cpp
    src/raycasting/stochasticRaytracing/Basistrimcrad.cpp
    src/raycasting/stochasticRaytracing/Ccr.cpp
    src/raycasting/stochasticRaytracing/Localline.cpp
    src/raycasting/stochasticRaytracing/Stochjacobi.cpp
    src/raycasting/stochasticRaytracing/LightSourceTable.cpp
    src/raycasting/stochasticRaytracing/Nondiff.cpp
    src/raycasting/stochasticRaytracing/Sample4d.cpp
    src/raycasting/stochasticRaytracing/Mcrad.cpp
    src/raycasting/stochasticRaytracing/Tracepath.cpp
    src/raycasting/stochasticRaytracing/StochasticRaytracingConfiguration.cpp
    src/raycasting/stochasticRaytracing/HierarchyRefine.cpp
    src/raycasting/stochasticRaytracing/Basisquadmcrad.cpp
    src/app/Raytrace.cpp)

add_executable(rpk ${MAIN_SRC})

if(WITH_OPENGL)
    target_sources(rpk PRIVATE ${OPEN_GL_SRC})
endif()

if(WITH_MGF)
    target_sources(rpk PRIVATE ${MGF_SRC})
endif()

if(WITH_RAYTRACING)
    target_sources(rpk PRIVATE ${RAYTRACING_SRC})
    target_sources(rpk PRIVATE ${APP_RAYTRACING_SRC})
endif()

if(WITH_RAYTRACING)
    target_compile_definitions(rpk PRIVATE RAYTRACING_ENABLED=true)
endif()

if(WITH_OPENGL)
    target_compile_definitions(rpk PRIVATE OPEN_GL_ENABLED=true)
endif()

if(WITH_MGF)
    target_compile_definitions(rpk PRIVATE MGF_ENABLED=true)
endif()

if(WITH_OPENGL)
    if(APPLE)
        message(STATUS "Using native macOS frameworks: ${OPENGL_FRAMEWORK}, ${GLUT_FRAMEWORK}")
        target_compile_definitions(rpk PRIVATE GL_SILENCE_DEPRECATION)
        target_link_libraries(rpk ${OPENGL_FRAMEWORK} ${GLUT_FRAMEWORK})
    else()
        target_link_libraries(rpk ${RPK_OPENGL_GLU_LIBRARY} ${RPK_OPENGL_GL_LIBRARY} ${RPK_OPENGL_GLUT_LIBRARY})
    endif()
endif()

if(APPLE)
    # Optional macOS post-build signing for local profiling tools (xctrace/leaks).
    # Create a signing identity in Xcode:
    # Xcode -> Settings -> Accounts -> select Apple ID -> Manage Certificates... -> "+" -> Apple Development.
    # This signature is only needed to make the built binary debuggable/profileable.
    # If no identity exists (or signing fails), build continues and the unsigned binary remains valid.
    add_custom_command(
        TARGET rpk
        POST_BUILD
        COMMAND /bin/sh -c "IDENTITY=$(/usr/bin/security find-identity -v -p codesigning \"$HOME/Library/Keychains/login.keychain-db\" 2>/dev/null | /usr/bin/sed -n 's/.*\"\\(.*\\)\"/\\1/p' | /usr/bin/head -n 1); if [ -n \"$IDENTITY\" ]; then /usr/bin/codesign --force --sign \"$IDENTITY\" \"$<TARGET_FILE:rpk>\" >/dev/null 2>&1 || true; fi"
        VERBATIM
    )
endif()
