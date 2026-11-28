# Java Notes

## Library layout

The build (`settings.gradle` of this folder) stacks four Gradle modules:

| Folder | Module | Contents |
| --- | --- | --- |
| `base/` | `:base` | Only the VITRAL files RenderPark takes, byte-identical to `~/VITRAL/vitral/java/base/src/main` (list in `scripts/vitralSubset.txt`) |
| `awt/` | `:awt` | VITRAL's AWT image persistence plugin (`ImagePersistenceAwt`, PNG output through `javax.imageio`), byte-identical to `~/VITRAL/vitral/java/awt/src/main` (list in `scripts/vitralAwtSubset.txt`); the program only needs it at run time |
| `rpkBase/` | `:rpkBase` | RenderPark's own classes: Galerkin, stochastic relaxation and random walk radiosity, raytracers and raycasters, scene model, MGF reader, tone mapping, support classes; it also builds the native random number generator of `pkgs/` |
| `rpkOpenGL1/` | `:rpkOpenGL1` | Interactive viewer and visual debug tools over fixed-function OpenGL 1 (JOGL) |
| `testsuite/ApplicationCases/RenderparkApplication/` | `:testsuite:ApplicationCases:RenderparkApplication` | The program: command line, options and the rendering driver |

`rpkBase` and `rpkOpenGL1` keep the `vsdk.toolkit...` packages their files had inside `base/`.
`scripts/vitralSubset.sh --check ~/VITRAL/vitral` compares `base/` and `awt/` with VITRAL byte for byte and fails on
any file of their `src/main` that is not in its list; a VITRAL file changes in VITRAL first and comes in
with `--refresh`. No VITRAL JOGL file (`java/jogl2`, `java/jogl4` in VITRAL) is used, so there is no
VITRAL OpenGL copy here.

RenderPark's lineal algebra is VITRAL's: the immutable `Vector2Df`, `Vector3Df`, `Vector4Df`,
`Matrix4x4f` and `Matrix2x2`. A method that the C++ port writes with a vector output pointer either
returns the vector or takes a one-element array as output holder.

## Image output through VITRAL

As in the C++ port, the images are written by VITRAL's `ImagePersistence`: `render/ImageOutputTarget`
chooses the format from the file name extension (`.png` and `.ppm` for display RGB, `.pic` for the high
dynamic range radiances in Radiance's RGBE format, `ImagePersistenceRgbe` over a
`RGBImageFloatUncompressed`), and `ScreenBuffer.writeFile` fills the image (tone mapped and gamma
corrected for display RGB). RenderPark's own `DkColor` and image output handles were removed, as were
its copies of `CoordinateSystem`, `CoordinateAxis` and `PersistenceElement`, which are VITRAL's now
(`CoordinateSystem.setFromZAxis` and `rectangularToSphericalCoord` take their epsilon as a parameter);
`Numeric` moved to `vsdk.toolkit.skin`. VITRAL's Java port writes PNG through a plugin found by
reflection, `ImagePersistenceAwt`, so the program needs the `:awt` module on its class path (a run
without it logs "Helper class not available, not saving PNG image" and leaves an empty file). The
corpus writes `output/<image>.png`, compared with the PNG golden images of the C++ port.

## Parallel final tracing after Random Walk

`-gr-threads N` enables parallel final tracing with `RandomWalk` and
`StochasticRaytracing` when `N > 1`. To run the corpus case from `java/`:

```bash
./scripts/runAllParallel.sh --threads 4 13_floorStochasticRaytracingRandomWalk
```

The radiosity iterations run serially. The final trace requires BRDF sampling
and falls back to serial tracing with stored radiance, coherent or correlated
sampling, or the legacy voxel grid. Each pixel has its own sample
sequence, so parallel results are reproducible across worker counts. The
historical serial render uses a different sequence.

## Running with JOGL

To run the JOGL demo with JNI native libraries:

```bash
./gradlew --stop
gradle --no-daemon :rpkOpenGL1:runHelloWorldJOGL
```
