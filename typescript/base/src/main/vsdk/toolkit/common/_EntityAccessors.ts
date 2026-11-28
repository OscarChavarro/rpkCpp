/**
Reflection data that `Entity.getEncapsulatedVariables()` reads in Java, for the
classes of the Java core that extend `Entity`: stages 1 and 2 of the Java
method, that is, the public getters without parameters whose return type is
supported, and the public one-parameter `void` setters, with their Java type
names. TypeScript keeps no parameter or return types at run time, so this
table stands in for `Class.getMethods()`.

Generated with `Class.getMethods()` on JDK 17 from the compiled Java core on
2026-11-28 (117 classes). The JDK does not specify the order of
`getMethods()`, and it changes between runs, so the entries are sorted by name.
Each class lists "Name:type" getters and setters.
*/
export const ENTITY_ACCESSORS: Readonly<
    Record<string, { readonly getters: readonly string[]; readonly setters: readonly string[] }>
> = {
    Entity: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    FundamentalEntity: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ColorRgb: {
        getters: ["B:double", "G:double", "R:double", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ArrayListOfBytes: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ArrayListOfDoubles: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ArrayListOfInts: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ArrayListOfLongs: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    BinaryTreeNode: {
        getters: ["SizeInBytes:int"],
        setters: [
            "Child:vsdk.toolkit.common.dataStructures.BinaryTreeNode",
            "ControlSpecifications:java.util.List",
            "Data:java.lang.Object",
            "Sibling:vsdk.toolkit.common.dataStructures.BinaryTreeNode",
        ],
    },
    CircularArrayList: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    NAryTree: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Root:vsdk.toolkit.common.dataStructures._NAryTreeNode"],
    },
    NAryTreeTraverser: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    _NAryTreeIntermediateNode: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Data:java.lang.Object"],
    },
    _NAryTreeLeafNode: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Data:java.lang.Object"],
    },
    _NAryTreeNode: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Data:java.lang.Object"],
    },
    Complex: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Matrix2x2: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Matrix4x4d: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Matrix4x4f: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    MatrixNxM: {
        getters: ["NumColumns:int", "NumRows:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Quaterniond: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Quaternionf: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Vector2Dd: {
        getters: ["SizeInBytes:int"],
        setters: [":vsdk.toolkit.common.linealAlgebra.Vector2Dd", "ControlSpecifications:java.util.List"],
    },
    Vector2Df: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Vector3Dd: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Vector3Df: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Vector4Dd: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Vector4Df: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    AlgebraicExpression: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Expression:java.lang.String"],
    },
    _AlgebraicExpressionBinaryOperatorNode: {
        getters: ["SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "LeftOperand:vsdk.toolkit.common.symbolicAlgebra._AlgebraicExpressionNode",
            "RightOperand:vsdk.toolkit.common.symbolicAlgebra._AlgebraicExpressionNode",
        ],
    },
    _AlgebraicExpressionConstantNode: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    _AlgebraicExpressionNode: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    _AlgebraicExpressionUnaryOperatorNode: {
        getters: ["SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "Operand:vsdk.toolkit.common.symbolicAlgebra._AlgebraicExpressionNode",
        ],
    },
    _AlgebraicExpressionVariableNode: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Background: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    CubemapBackground: {
        getters: ["SizeInBytes:int"],
        setters: ["Camera:vsdk.toolkit.environment.camera.Camera", "ControlSpecifications:java.util.List"],
    },
    FixedBackground: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Image:vsdk.toolkit.media.RGBAImageUncompressed"],
    },
    SimpleBackground: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Camera: {
        getters: [
            "FarPlaneDistance:double",
            "FocusedPosition:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Fov:double",
            "Front:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Left:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "ModificationVersion:long",
            "Name:java.lang.String",
            "NearPlaneDistance:double",
            "NormalizingTransformation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "OrthogonalZoom:double",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "ProjectionMode:int",
            "RightWithScale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "SizeInBytes:int",
            "Up:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "UpWithScale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "ViewportXSize:double",
            "ViewportYSize:double",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "FarPlaneDistance:double",
            "FocusedPositionDirect:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "FocusedPositionMaintainingOrthogonality:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Fov:double",
            "LeftDirect:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Name:java.lang.String",
            "NearPlaneDistance:double",
            "OrthogonalZoom:double",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "ProjectionMode:int",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "UpDirect:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "UpMaintainingOrthogonality:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    Geometry: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Curve: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ParametricCurve: {
        getters: ["ApproximationSteps:int", "NumPieces:int", "PointSize:int", "SizeInBytes:int"],
        setters: ["ApproximationSteps:int", "ControlSpecifications:java.util.List"],
    },
    PlanarFace: {
        getters: [
            "Area:double",
            "DominantAxis:int",
            "JacobianA:double",
            "JacobianB:double",
            "JacobianC:double",
            "MidPoint:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Normal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "NumberOfVertices:int",
            "PlaneConstant:double",
            "SizeInBytes:int",
            "Tolerance:double",
        ],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Ray: {
        getters: [
            "Direction:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Origin:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
            "T:double",
        ],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RayHit: {
        getters: ["AvailableDetailMask:int", "HitDistance:double", "RequiredDetailMask:int", "SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "HitDistance:double",
            "Ray:vsdk.toolkit.environment.geometry.element.Ray",
            "RequiredDetailMask:int",
            "StoreRay:boolean",
        ],
    },
    Triangle: {
        getters: [
            "Normal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Point0:int",
            "Point1:int",
            "Point2:int",
            "SizeInBytes:int",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Normal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Point0:int",
            "Point1:int",
            "Point2:int",
        ],
    },
    Vertex: {
        getters: [
            "Binormal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Normal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
            "Tangent:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "U:double",
            "V:double",
        ],
        setters: [
            "Binormal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "ControlSpecifications:java.util.List",
            "IncidentTriangles:java.util.ArrayList",
            "Normal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Tangent:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "U:double",
            "V:double",
        ],
    },
    Vertex2D: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    FunctionalExplicitSurface: {
        getters: [
            "FunctionExpression:java.lang.String",
            "MaxXBound:double",
            "MaxYBound:double",
            "MaxZBound:double",
            "MinXBound:double",
            "MinYBound:double",
            "MinZBound:double",
            "SizeInBytes:int",
            "TesselationHintX:int",
            "TesselationHintY:int",
        ],
        setters: ["ControlSpecifications:java.util.List"],
    },
    HalfSpace: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    InfinitePlane: {
        getters: [
            "A:double",
            "B:double",
            "C:double",
            "Coefficients:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "ConstantCoefficient:double",
            "D:double",
            "Normal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
        ],
        setters: [
            "A:double",
            "B:double",
            "C:double",
            "ControlSpecifications:java.util.List",
            "D:double",
            "Normal:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    ParametricBiCubicPatch: {
        getters: ["ApproximationSteps:int", "SizeInBytes:int", "Type:int"],
        setters: ["ApproximationSteps:int", "ControlSpecifications:java.util.List", "Type:int"],
    },
    QuadMesh: {
        getters: ["Name:java.lang.String", "NumQuads:int", "NumVertices:int", "SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "Name:java.lang.String",
            "Vertexes:[Lvsdk.toolkit.environment.geometry.element.Vertex;",
        ],
    },
    SubdividedBezierPatch: {
        getters: ["Flatness:double", "SizeInBytes:int", "USteps:int", "VSteps:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Surface: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    TriangleMesh: {
        getters: ["Name:java.lang.String", "NumTriangles:int", "NumVertices:int", "SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "DoubleSided:boolean",
            "MaterialRanges:[[I",
            "Materials:[Lvsdk.toolkit.environment.material.SimpleMaterial;",
            "Name:java.lang.String",
            "TextureRanges:[[I",
            "Textures:[Lvsdk.toolkit.media.Image;",
            "Triangles:[Lvsdk.toolkit.environment.geometry.element.Triangle;",
        ],
    },
    TriangleMeshGroup: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Meshes:java.util.ArrayList"],
    },
    TriangleStripMesh: {
        getters: ["SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "Strips:[[I",
            "Vertexes:[Lvsdk.toolkit.environment.geometry.element.Vertex;",
        ],
    },
    Polygon2D: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "HeadNode:vsdk.toolkit.common.dataStructures.BinaryTreeNode"],
    },
    _Polygon2DContour: {
        getters: ["SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "ExteriorContour:vsdk.toolkit.environment.geometry.surface.polygon._Polygon2DContour",
        ],
    },
    Arrow: {
        getters: [
            "BaseLength:double",
            "BaseRadius:double",
            "HeadLength:double",
            "HeadRadius:double",
            "SizeInBytes:int",
        ],
        setters: [
            "BaseLength:double",
            "BaseRadius:double",
            "ControlSpecifications:java.util.List",
            "HeadLength:double",
            "HeadRadius:double",
        ],
    },
    BlobSolid: {
        getters: ["ComponentCount:long", "SizeInBytes:int", "Threshold:double"],
        setters: ["ControlSpecifications:java.util.List", "Threshold:double"],
    },
    Box: {
        getters: [
            "Center:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Size:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
            "SizeX:double",
            "SizeY:double",
            "SizeZ:double",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Size:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeX:double",
            "SizeY:double",
            "SizeZ:double",
        ],
    },
    Cone: {
        getters: ["BottomRadius:double", "Height:double", "SizeInBytes:int", "TopRadius:double"],
        setters: ["BottomRadius:double", "ControlSpecifications:java.util.List", "Height:double", "TopRadius:double"],
    },
    HeightFieldSolid: {
        getters: [
            "ColumnCount:long",
            "Maximum:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Minimum:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "RowCount:long",
            "SizeInBytes:int",
        ],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ImplicitSolid: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    PlanarHalfSpace: {
        getters: ["A:double", "B:double", "C:double", "D:double", "SizeInBytes:int"],
        setters: ["A:double", "B:double", "C:double", "ControlSpecifications:java.util.List", "D:double"],
    },
    PolynomialSurface: {
        getters: ["Order:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Prism: {
        getters: ["Circumradius:double", "Height:double", "NumberOfSides:int", "SizeInBytes:int"],
        setters: [
            "Base:java.util.ArrayList",
            "Circumradius:double",
            "ControlSpecifications:java.util.List",
            "Height:double",
            "NumberOfSides:int",
        ],
    },
    QuadricSurface: {
        getters: [
            "Constant:double",
            "LinearX:double",
            "LinearY:double",
            "LinearZ:double",
            "MixedXY:double",
            "MixedXZ:double",
            "MixedYZ:double",
            "SizeInBytes:int",
            "SquareX:double",
            "SquareY:double",
            "SquareZ:double",
        ],
        setters: [
            "Constant:double",
            "ControlSpecifications:java.util.List",
            "LinearX:double",
            "LinearY:double",
            "LinearZ:double",
            "MixedXY:double",
            "MixedXZ:double",
            "MixedYZ:double",
            "SquareX:double",
            "SquareY:double",
            "SquareZ:double",
        ],
    },
    Solid: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Sphere: {
        getters: ["Radius:double", "RadiusReference:double", "RadiusSquared:double", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Radius:double"],
    },
    Torus: {
        getters: ["MajorRadius:double", "MinorRadius:double", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "MajorRadius:double", "MinorRadius:double"],
    },
    Tube: {
        getters: ["ExternalRadius:double", "Height:double", "InternalRadius:double", "SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "ExternalRadius:double",
            "Height:double",
            "InternalRadius:double",
        ],
    },
    Volume: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    VoxelVolume: {
        getters: ["SizeInBytes:int", "Threshold:int", "XSize:int", "YSize:int", "ZSize:int"],
        setters: ["ControlSpecifications:java.util.List", "Threshold:int"],
    },
    PolyhedralBoundedSolid: {
        getters: ["MaxFaceId:int", "MaxVertexId:int", "SizeInBytes:int"],
        setters: [
            "ControlSpecifications:java.util.List",
            "EdgesList:vsdk.toolkit.common.dataStructures.CircularArrayList",
            "MaxFaceId:int",
            "MaxVertexId:int",
            "PolygonsList:vsdk.toolkit.common.dataStructures.CircularArrayList",
            "VerticesList:vsdk.toolkit.common.dataStructures.CircularArrayList",
        ],
    },
    _PolyhedralBoundedSolidEdge: {
        getters: ["EndingVertexId:int", "ListIndex:int", "SizeInBytes:int", "StartingVertexId:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    _PolyhedralBoundedSolidFace: {
        getters: ["ListIndex:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    _PolyhedralBoundedSolidHalfEdge: {
        getters: ["ListIndex:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    _PolyhedralBoundedSolidLoop: {
        getters: ["ListIndex:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    _PolyhedralBoundedSolidVertex: {
        getters: ["ListIndex:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    AmbientLight: {
        getters: [
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    DirectionalLight: {
        getters: [
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    Light: {
        getters: [
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    PointLight: {
        getters: [
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    SpotLight: {
        getters: [
            "Coefficient:double",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Falloff:double",
            "Id:int",
            "Name:java.lang.String",
            "PointsAt:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Radius:double",
            "SizeInBytes:int",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "Id:int",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    MicroFacetedMaterial: {
        getters: [
            "Alpha:double",
            "Ambient:vsdk.toolkit.common.color.ColorRgb",
            "AmbientReference:vsdk.toolkit.common.color.ColorRgb",
            "Diffuse:vsdk.toolkit.common.color.ColorRgb",
            "DiffuseReference:vsdk.toolkit.common.color.ColorRgb",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "EmissionReference:vsdk.toolkit.common.color.ColorRgb",
            "Eta:vsdk.toolkit.common.color.ColorRgb",
            "FresnelF0:vsdk.toolkit.common.color.ColorRgb",
            "FresnelModel:int",
            "GeometryModel:int",
            "IndexOfRefraction:double",
            "Kappa:vsdk.toolkit.common.color.ColorRgb",
            "Kd:double",
            "Ks:double",
            "Name:java.lang.String",
            "NdfModel:int",
            "Opacity:double",
            "PhongExponent:double",
            "ReflectionCoefficient:double",
            "RefractionCoefficient:double",
            "Roughness:double",
            "SizeInBytes:int",
            "Specular:vsdk.toolkit.common.color.ColorRgb",
            "SpecularReference:vsdk.toolkit.common.color.ColorRgb",
            "Transmittance:vsdk.toolkit.common.color.ColorRgb",
            "TransmittanceReference:vsdk.toolkit.common.color.ColorRgb",
        ],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RendererConfiguration: {
        getters: [
            "BoundingVolumeColor:vsdk.toolkit.common.color.ColorRgb",
            "LodHint:int",
            "ShadingType:int",
            "SizeInBytes:int",
            "UseVertexColors:boolean",
            "VertexNormalSmoothingThresholdDegrees:double",
            "WireColor:vsdk.toolkit.common.color.ColorRgb",
        ],
        setters: [
            "BoundingVolume:boolean",
            "BoundingVolumeColor:vsdk.toolkit.common.color.ColorRgb",
            "BumpMap:boolean",
            "ControlSpecifications:java.util.List",
            "LodHint:int",
            "MeshAccelerationMethod:vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.TriangleMeshAccelerationMethod",
            "Normals:boolean",
            "Points:boolean",
            "SelectionCorners:boolean",
            "ShadingType:int",
            "ShadingType:vsdk.toolkit.environment.material.ShadingType",
            "Surfaces:boolean",
            "Texture:boolean",
            "TrianglesNormals:boolean",
            "UseVertexColors:boolean",
            "VertexNormalSmoothingThresholdDegrees:double",
            "WireColor:vsdk.toolkit.common.color.ColorRgb",
            "Wires:boolean",
            "WithShadows:boolean",
        ],
    },
    SimpleMaterial: {
        getters: [
            "Ambient:vsdk.toolkit.common.color.ColorRgb",
            "AmbientReference:vsdk.toolkit.common.color.ColorRgb",
            "Diffuse:vsdk.toolkit.common.color.ColorRgb",
            "DiffuseReference:vsdk.toolkit.common.color.ColorRgb",
            "Emission:vsdk.toolkit.common.color.ColorRgb",
            "EmissionReference:vsdk.toolkit.common.color.ColorRgb",
            "IndexOfRefraction:double",
            "Name:java.lang.String",
            "Opacity:double",
            "PhongExponent:double",
            "ReflectionCoefficient:double",
            "RefractionCoefficient:double",
            "SizeInBytes:int",
            "Specular:vsdk.toolkit.common.color.ColorRgb",
            "SpecularReference:vsdk.toolkit.common.color.ColorRgb",
            "Transmittance:vsdk.toolkit.common.color.ColorRgb",
            "TransmittanceReference:vsdk.toolkit.common.color.ColorRgb",
        ],
        setters: ["ControlSpecifications:java.util.List"],
    },
    SceneSolid: {
        getters: [
            "AffinePlacement:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "ModificationVersion:long",
            "Name:java.lang.String",
            "SizeInBytes:int",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Geometry:vsdk.toolkit.environment.geometry.Geometry",
            "GeometryReference:vsdk.toolkit.environment.geometry.Geometry",
            "Name:java.lang.String",
        ],
    },
    SimpleBody: {
        getters: [
            "AffinePlacement:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "ModificationVersion:long",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "RotationInverse:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "Scale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
            "TransformationMatrix:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Geometry:vsdk.toolkit.environment.geometry.Geometry",
            "GeometryReference:vsdk.toolkit.environment.geometry.Geometry",
            "Material:vsdk.toolkit.environment.material.SimpleMaterial",
            "MaterialReference:vsdk.toolkit.environment.material.SimpleMaterial",
            "Name:java.lang.String",
            "NormalMap:vsdk.toolkit.media.NormalMap",
            "NormalMapReference:vsdk.toolkit.media.NormalMap",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "RotationInverse:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "Scale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Texture:vsdk.toolkit.media.Image",
            "TextureReference:vsdk.toolkit.media.Image",
        ],
    },
    SimpleBodyGroup: {
        getters: [
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "RotationInverse:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "Scale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
            "TransformationMatrix:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Name:java.lang.String",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "RotationInverse:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "Scale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
        ],
    },
    SimpleScene: {
        getters: ["ActiveBackgroundIndex:int", "ActiveCameraIndex:int", "SizeInBytes:int"],
        setters: [
            "ActiveBackgroundIndex:int",
            "ActiveCameraIndex:int",
            "Backgrounds:java.util.ArrayList",
            "Cameras:java.util.ArrayList",
            "ControlSpecifications:java.util.List",
            "Lights:java.util.ArrayList",
            "SimpleBodies:java.util.ArrayList",
        ],
    },
    SimpleSceneSnapshot: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    CsgBody: {
        getters: [
            "AffinePlacement:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "LeafCount:long",
            "MembershipTolerance:double",
            "ModificationVersion:long",
            "Name:java.lang.String",
            "NodeCount:long",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "RotationInverse:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "Scale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
            "TransformationMatrix:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
        ],
        setters: [
            "ControlSpecifications:java.util.List",
            "Geometry:vsdk.toolkit.environment.geometry.Geometry",
            "GeometryReference:vsdk.toolkit.environment.geometry.Geometry",
            "Material:vsdk.toolkit.environment.material.SimpleMaterial",
            "MaterialReference:vsdk.toolkit.environment.material.SimpleMaterial",
            "Name:java.lang.String",
            "NormalMap:vsdk.toolkit.media.NormalMap",
            "NormalMapReference:vsdk.toolkit.media.NormalMap",
            "Position:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Rotation:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "RotationInverse:vsdk.toolkit.common.linealAlgebra.Matrix4x4d",
            "Scale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "Texture:vsdk.toolkit.media.Image",
            "TextureReference:vsdk.toolkit.media.Image",
        ],
    },
    Calligraphic2DBuffer: {
        getters: ["NumLines:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Calligraphic3DBuffer: {
        getters: ["NumLines:int", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    FourierShapeDescriptor: {
        getters: ["Label:java.lang.String", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "FeatureVector:[D", "Label:java.lang.String"],
    },
    GeometryMetadata: {
        getters: ["Filename:java.lang.String", "Id:long", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "Filename:java.lang.String", "Id:long"],
    },
    GrayScalePalette: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    Image: {
        getters: ["SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    IndexedColorImageUncompressed: {
        getters: ["SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ColorTable:vsdk.toolkit.media.RGBColorPalette", "ControlSpecifications:java.util.List"],
    },
    MediaEntity: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    NormalMap: {
        getters: [
            "BumpMapScale:vsdk.toolkit.common.linealAlgebra.Vector3Dd",
            "SizeInBytes:int",
            "XSize:int",
            "YSize:int",
        ],
        setters: ["ControlSpecifications:java.util.List"],
    },
    NormalMapTexture: {
        getters: ["RawImageSizeInBytes:int", "SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    PrimitiveCountShapeDescriptor: {
        getters: ["Label:java.lang.String", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "FeatureVector:[D", "Label:java.lang.String"],
    },
    RGBAColorPalette: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RGBAImageCompressed: {
        getters: ["CompressedDataSize:int", "CompressionFormat:int", "SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RGBAImageUncompressed: {
        getters: ["RawImageSizeInBytes:int", "SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RGBAPixel: {
        getters: ["A:byte", "B:byte", "G:byte", "R:byte", "SizeInBytes:int"],
        setters: ["A:byte", "B:byte", "ControlSpecifications:java.util.List", "G:byte", "R:byte"],
    },
    RGBColorPalette: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RGBImageFloatUncompressed: {
        getters: ["SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RGBImageUncompressed: {
        getters: ["RawImageSizeInBytes:int", "SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    RGBPixel: {
        getters: ["B:byte", "G:byte", "R:byte", "SizeInBytes:int"],
        setters: ["B:byte", "ControlSpecifications:java.util.List", "G:byte", "R:byte"],
    },
    RGBProceduralColorPalette: {
        getters: ["SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List"],
    },
    ShapeDescriptor: {
        getters: ["Label:java.lang.String", "SizeInBytes:int"],
        setters: ["ControlSpecifications:java.util.List", "FeatureVector:[D", "Label:java.lang.String"],
    },
    ZBuffer: {
        getters: ["SizeInBytes:int", "XSize:int", "YSize:int"],
        setters: ["ControlSpecifications:java.util.List", "ZBuffer:[F"],
    },
};
