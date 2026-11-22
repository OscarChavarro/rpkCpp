import type { Ray } from "../common/linealAlgebra/Ray";
import { RayExclusionSet } from "../environment/geometry/elements/RayExclusionSet";
import type { RayHit } from "../environment/geometry/elements/RayHit";
import type { Geometry } from "../skin/Geometry";

const fs = require("node:fs");

/**
Ray accounting and ray corpus capture of the visibility queries of one run.
Only runs with the RPK_RAY_CAPTURE environment variable set (the TypeScript
equivalent of the C++ WITH_RAY_CAPTURE build) create one, so the default run
is unchanged.

It counts where each shadow ray of the Galerkin solver is answered (shadow
cache, restricted geometry list produced by shaft culling, or full-scene
index) and writes every list and index query, with its answer, to a binary
corpus file that later index implementations are replayed against.

Corpus format, all fields 32-bit little-endian, byte compatible with the
corpus written by the C++ VisibilityQueryRecorder:

    header   char magic[8] = "RPKVQC01", int32 version = 1
    record   int32 tag, then by tag:
      1 GEOMETRY_LIST  int32 count, int32 geometryId[count]
                       The candidate list of the restricted-list queries
                       that follow, until the next GEOMETRY_LIST.
      2 QUERY          int32 path, int32 hitFlags,
                       float origin[3], float direction[3],
                       float minimumDistance, float maximumDistance,
                       int32 excludedPatchId[4] (0 = none),
                       int32 excludedGeometryId[2] (-1 = none),
                       int32 blockerPatchId (0 = no hit),
                       float blockerDistance (maximumDistance when no hit)

Paths: 0 Galerkin restricted list, 1 Galerkin full-scene index,
2 raytracer closest hit, 3 raytracer visibility, 4 stochastic radiosity
shooting. Patch identifiers are Patch.id (from 1), geometry identifiers
Geometry.id, both deterministic for a given scene and options.

Workspace role: owned by the scene, reached through the caller's
SceneQueryWorkspace.
*/
export class VisibilityQueryRecorder {
  public static readonly GALERKIN_RESTRICTED_LIST = 0;
  public static readonly GALERKIN_FULL_SCENE = 1;
  public static readonly RAYTRACER_CLOSEST_HIT = 2;
  public static readonly RAYTRACER_VISIBILITY = 3;
  public static readonly STOCHASTIC_SHOOTING = 4;
  public static readonly NUMBER_OF_PATHS = 5;

  private static readonly BUFFER_SIZE = 1 << 16;

  private corpus: number | null;
  private corpusFileName: string | null;
  private cacheResolved: number;
  private readonly queries: number[];
  private readonly hits: number[];
  private geometryLists: number;
  private readonly buffer: Uint8Array;
  private readonly view: DataView;
  private bufferUsed: number;

  private static pathName(path: number): string {
    switch (path) {
      case VisibilityQueryRecorder.GALERKIN_RESTRICTED_LIST:
        return "Galerkin restricted list";
      case VisibilityQueryRecorder.GALERKIN_FULL_SCENE:
        return "Galerkin full-scene index";
      case VisibilityQueryRecorder.RAYTRACER_CLOSEST_HIT:
        return "raytracer closest hit";
      case VisibilityQueryRecorder.RAYTRACER_VISIBILITY:
        return "raytracer visibility";
      default:
        return "stochastic shooting";
    }
  }

  /**
  Creates the recorder of a run when RPK_RAY_CAPTURE is set (to anything but
  an empty string or "0"), writing the corpus to RPK_RAY_CAPTURE_FILE when it
  names a file; returns null otherwise
  */
  public static fromEnvironment(): VisibilityQueryRecorder | null {
    const capture = process.env["RPK_RAY_CAPTURE"];
    if (capture === undefined || capture === "" || capture === "0") {
      return null;
    }
    return new VisibilityQueryRecorder(process.env["RPK_RAY_CAPTURE_FILE"] ?? null);
  }

  public constructor(inCorpusFileName: string | null) {
    this.corpus = null;
    this.corpusFileName = null;
    this.cacheResolved = 0;
    this.queries = new Array<number>(VisibilityQueryRecorder.NUMBER_OF_PATHS).fill(0);
    this.hits = new Array<number>(VisibilityQueryRecorder.NUMBER_OF_PATHS).fill(0);
    this.geometryLists = 0;
    this.buffer = new Uint8Array(VisibilityQueryRecorder.BUFFER_SIZE);
    this.view = new DataView(this.buffer.buffer);
    this.bufferUsed = 0;
    if (inCorpusFileName !== null && inCorpusFileName !== "") {
      this.corpusFileName = inCorpusFileName;
      try {
        this.corpus = fs.openSync(this.corpusFileName, "w") as number;
      }
      catch {
        this.corpus = null;
      }
      if (this.corpus === null) {
        process.stderr.write(`Ray capture: cannot open corpus file ${this.corpusFileName}, only counting\n`);
      }
      else {
        const magic = "RPKVQC01";
        this.reserve(magic.length);
        for (let i = 0; i < magic.length; i++) {
          this.buffer[this.bufferUsed++] = magic.charCodeAt(i);
        }
        this.writeInt(1);
      }
    }
  }

  /**
  Prints the summary and closes the corpus (the C++ destructor)
  */
  public destroy(): void {
    this.printSummary();
    if (this.corpus !== null) {
      this.flush();
      fs.closeSync(this.corpus);
      this.corpus = null;
    }
    this.corpusFileName = null;
  }

  private flush(): void {
    let written = 0;
    while (this.corpus !== null && written < this.bufferUsed) {
      written += fs.writeSync(this.corpus, this.buffer, written, this.bufferUsed - written) as number;
    }
    this.bufferUsed = 0;
  }

  private reserve(bytes: number): void {
    if (this.bufferUsed + bytes > this.buffer.length) {
      this.flush();
    }
  }

  private writeInt(value: number): void {
    this.reserve(4);
    this.view.setInt32(this.bufferUsed, value | 0, true);
    this.bufferUsed += 4;
  }

  private writeFloat(value: number): void {
    this.reserve(4);
    this.view.setFloat32(this.bufferUsed, value, true);
    this.bufferUsed += 4;
  }

  public countCacheResolved(): void {
    this.cacheResolved++;
  }

  public recordGeometryList(geometryList: Geometry[] | null): void {
    this.geometryLists++;
    if (this.corpus === null) {
      return;
    }
    const count = geometryList !== null ? geometryList.length : 0;
    this.writeInt(1);
    this.writeInt(count);
    for (let i = 0; i < count; i++) {
      this.writeInt(geometryList![i]!.id);
    }
  }

  public recordQuery(
    path: number,
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number,
    hitFlags: number,
    exclusions: RayExclusionSet,
    hit: RayHit | null,
    hitDistance: number
  ): void {
    this.queries[path]!++;
    if (hit !== null) {
      this.hits[path]!++;
    }
    if (this.corpus === null) {
      return;
    }
    this.writeInt(2);
    this.writeInt(path);
    this.writeInt(hitFlags);
    this.writeFloat(ray.position.x);
    this.writeFloat(ray.position.y);
    this.writeFloat(ray.position.z);
    this.writeFloat(ray.direction.x);
    this.writeFloat(ray.direction.y);
    this.writeFloat(ray.direction.z);
    this.writeFloat(minimumDistance);
    this.writeFloat(maximumDistance);
    for (let i = 0; i < RayExclusionSet.MAX_EXCLUDED_PATCHES; i++) {
      const patch = exclusions.getExcludedPatch(i);
      this.writeInt(patch !== null ? patch.id : 0);
    }
    const geometry1 = exclusions.getExcludedGeometry1();
    const geometry2 = exclusions.getExcludedGeometry2();
    this.writeInt(geometry1 !== null ? geometry1.id : -1);
    this.writeInt(geometry2 !== null ? geometry2.id : -1);
    const blocker = hit !== null ? hit.getPatch() : null;
    this.writeInt(blocker !== null ? blocker.id : 0);
    this.writeFloat(hit !== null ? hitDistance : maximumDistance);
  }

  public printSummary(): void {
    process.stderr.write(`Ray capture: shadow rays resolved by the shadow cache: ${this.cacheResolved}\n`);
    for (let i = 0; i < VisibilityQueryRecorder.NUMBER_OF_PATHS; i++) {
      process.stderr.write(
        `Ray capture: ${VisibilityQueryRecorder.pathName(i)} queries: ${this.queries[i]}, with hit: ${this.hits[i]}\n`
      );
    }
    process.stderr.write(`Ray capture: restricted geometry lists: ${this.geometryLists}\n`);
    if (this.corpusFileName !== null) {
      process.stderr.write(`Ray capture: corpus written to ${this.corpusFileName}\n`);
    }
  }
}
