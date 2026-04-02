cmake_minimum_required(VERSION 3.10)
project(rpk)

option(RAYTRACING_ENABLED "Enable raytracing modules" ON)
option(OPEN_GL_ENABLED "Enable OpenGL modules" ON)
option(MGF_ENABLED "Enable MGF reader modules" ON)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

if(OPEN_GL_ENABLED)
    if(APPLE)
        find_library(OPENGL_FRAMEWORK OpenGL)
        find_library(GLUT_FRAMEWORK GLUT)
        if(NOT OPENGL_FRAMEWORK OR NOT GLUT_FRAMEWORK)
            message(WARNING "OpenGL support requested, but OpenGL/GLUT frameworks were not found. Building without OpenGL support.")
            set(OPEN_GL_ENABLED OFF)
        endif()
    else()
        find_library(RPK_OPENGL_GL_LIBRARY GL)
        find_library(RPK_OPENGL_GLU_LIBRARY GLU)
        find_library(RPK_OPENGL_GLUT_LIBRARY glut)
        if(NOT RPK_OPENGL_GL_LIBRARY OR NOT RPK_OPENGL_GLU_LIBRARY OR NOT RPK_OPENGL_GLUT_LIBRARY)
            message(WARNING "OpenGL support requested, but GL/GLU/GLUT libraries were not found. Building without OpenGL support.")
            set(OPEN_GL_ENABLED OFF)
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
    src/common/Error.cpp
    src/common/CppReAlloc.cpp
    src/common/ColorRgb.cpp
    src/common/Cie.cpp
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
    src/sgl/PolyClip.cpp
    src/sgl/PolyScanZ.cpp
    src/sgl/SglContext.cpp
    src/sgl/PolyScanFlat.cpp
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
    src/io/context/ParseSession.cpp
    src/io/context/ParserConfig.cpp
    src/io/context/ReaderStackState.cpp
    src/io/context/GeometryBuildState.cpp
    src/io/context/MaterialState.cpp
    src/io/context/ColorRepository.cpp
    src/io/context/MaterialRepository.cpp
    src/io/context/VertexRepository.cpp
    src/io/context/ObjectHierarchyState.cpp
    src/io/context/TransformStack.cpp
    src/io/context/LookUpEntity.cpp
    src/io/context/LookUpTable.cpp
    src/io/context/ColorContext.cpp
    src/io/context/WordsContext.cpp
    src/io/context/PersistedSceneModel.cpp
    src/io/bin/writer/BinaryModelWriter.cpp
    src/io/bin/writer/BinaryModelWriterSerializationContext.cpp
    src/io/bin/reader/BinaryModelReader.cpp
    src/io/bin/reader/BinaryModelReaderSupport.cpp
    src/io/bin/reader/BinaryModelReaderCleanup.cpp
    src/io/bin/reader/BinaryModelReaderIndexListRecord.cpp
    src/io/bin/reader/BinaryModelReaderVertexRecord.cpp
    src/io/bin/reader/BinaryModelReaderPatchRecord.cpp
    src/io/bin/reader/BinaryModelReaderGeometryRecord.cpp
    src/io/bin/reader/BinaryModelReaderModelRecord.cpp
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
    src/render/Rendercommon.cpp
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
    src/app/OptionsType.cpp
    src/app/Options.cpp
    src/app/Radiance.cpp
    src/app/SceneBuilder.cpp
    src/app/Batch.cpp
    src/app/CommandLine.cpp
    src/app/BatchOptions.cpp
    src/app/RpkApplication.cpp
    src/app/Main.cpp)

set(OPEN_GL_SRC
    src/render/opengl/Opengl.cpp
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
    src/io/mgf/MgfDefinitions.cpp
    src/io/mgf/MgfGeometry.cpp
    src/io/mgf/MgfHandlerGeometry.cpp
    src/io/mgf/MgfHandlerObject.cpp
    src/io/mgf/MgfHandlerTransform.cpp
    src/io/mgf/MgfHandlerColor.cpp
    src/io/mgf/MgfHandlerMaterial.cpp
    src/io/mgf/MgfStaticHandler.cpp
    src/io/mgf/MgfReader.cpp)

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
    src/photonMap/SampleGrid2D.cpp
    src/photonMap/NormalQuery.cpp
    src/photonMap/DiscreteSampling.cpp
    src/photonMap/PhotonKDTree.cpp
    src/photonMap/PhotonMapImportance.cpp
    src/photonMap/ScreenSampler.cpp
    src/photonMap/PhotonMapState.cpp
    src/photonMap/Photon.cpp
    src/photonMap/ImportanceMap.cpp
    src/photonMap/PhotonMapRadianceMethod.cpp
    src/photonMap/PhotonMap.cpp
    src/photonMap/PhotonMapSampler.cpp
    src/raycasting/stochasticRaytracing/StochasticRaytracingState.cpp
    src/raycasting/stochasticRaytracing/StochasticRaytracer.cpp
    src/raycasting/stochasticRaytracing/StochasticRadiosityElement.cpp
    src/raycasting/stochasticRaytracing/Hierarchy.cpp
    src/raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.cpp
    src/raycasting/stochasticRaytracing/Render.cpp
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

if(OPEN_GL_ENABLED)
    target_sources(rpk PRIVATE ${OPEN_GL_SRC})
endif()

if(MGF_ENABLED)
    target_sources(rpk PRIVATE ${MGF_SRC})
endif()

if(RAYTRACING_ENABLED)
    target_sources(rpk PRIVATE ${RAYTRACING_SRC})
endif()

if(RAYTRACING_ENABLED)
    target_compile_definitions(rpk PRIVATE RAYTRACING_ENABLED)
endif()

if(OPEN_GL_ENABLED)
    target_compile_definitions(rpk PRIVATE OPEN_GL_ENABLED)
endif()

if(MGF_ENABLED)
    target_compile_definitions(rpk PRIVATE MGF_ENABLED)
endif()

if(OPEN_GL_ENABLED)
    if(APPLE)
        message(STATUS "Using native macOS frameworks: ${OPENGL_FRAMEWORK}, ${GLUT_FRAMEWORK}")
        target_compile_definitions(rpk PRIVATE GL_SILENCE_DEPRECATION)
        target_link_libraries(rpk ${OPENGL_FRAMEWORK} ${GLUT_FRAMEWORK})
    else()
        target_link_libraries(rpk ${RPK_OPENGL_GLU_LIBRARY} ${RPK_OPENGL_GL_LIBRARY} ${RPK_OPENGL_GLUT_LIBRARY})
    endif()
endif()
