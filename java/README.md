# Java Notes

## Library layout

The build (`settings.gradle` of this folder) stacks four Gradle modules:

| Folder | Module | Contents |
| --- | --- | --- |
| `base/` | `:base` | Only the VITRAL files RenderPark takes, byte-identical to `~/VITRAL/vitral/java/base/src/main` (list in `scripts/vitralSubset.txt`) |
| `rpkBase/` | `:rpkBase` | RenderPark's own classes: Galerkin, stochastic relaxation and random walk radiosity, raytracers and raycasters, scene model, MGF reader, tone mapping, support classes; it also builds the native random number generator of `pkgs/` |
| `rpkOpenGL1/` | `:rpkOpenGL1` | Interactive viewer and visual debug tools over fixed-function OpenGL 1 (JOGL) |
| `testsuite/ApplicationCases/RenderparkApplication/` | `:testsuite:ApplicationCases:RenderparkApplication` | The program: command line, options and the rendering driver |

`rpkBase` and `rpkOpenGL1` keep the `vsdk.toolkit...` packages their files had inside `base/`.
`scripts/vitralSubset.sh --check ~/VITRAL/vitral` compares `base/` with VITRAL byte for byte and fails on
any file of `base/src/main` that is not in the list; a VITRAL file changes in VITRAL first and comes in
with `--refresh`. No VITRAL JOGL file (`java/jogl2`, `java/jogl4` in VITRAL) is used, so there is no
VITRAL OpenGL copy here.

RenderPark's lineal algebra is VITRAL's: the immutable `Vector2Df`, `Vector3Df`, `Vector4Df`,
`Matrix4x4f` and `Matrix2x2`. A method that the C++ port writes with a vector output pointer either
returns the vector or takes a one-element array as output holder.

## Running with JOGL

To run the JOGL demo with JNI native libraries:

```bash
./gradlew --stop
gradle --no-daemon :rpkOpenGL1:runHelloWorldJOGL
```
