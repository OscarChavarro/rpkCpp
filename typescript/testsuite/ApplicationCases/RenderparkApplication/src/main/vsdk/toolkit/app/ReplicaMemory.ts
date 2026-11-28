import type { NodeWorkerOptions } from "rpk-base/dist/java/concurrent/NodeWorkerExecutor";

/*
Memory settings of the parallel Jacobi replicas (see RefinementReplicas).

Every replica is a whole copy of the program, so its garbage counts as many
times as there are threads. Left to its own heuristics, with the 16 GB old
space limit of runNodeProgram.mjs, V8 lets each heap grow to more than ten
times its live data before a full collection (measured on 01_cubeJacobi:
515 MB of heap for 35 MB alive), and gives every worker the young generation
of a large heap. Two settings keep the heaps close to their live data:
- a full garbage collection by every replica at the start of each iteration
  (RefinementReplicas.setMemoryReclaimer), through V8's gc function, enabled
  here at run time as --expose-gc does;
- a small young generation for the workers (their resourceLimits), so their
  scavenges stay cheap and short lived garbage does not take memory.
The old space of the workers keeps the limit of the main thread: a scene must
fit in every replica.
*/
export class ReplicaMemory {
  /** Young generation of each worker, in MB */
  private static readonly WORKER_YOUNG_GENERATION_MB = 16;

  private static collector: (() => void) | null | undefined = undefined;

  /** V8's full garbage collection, or null when the engine does not give it */
  public static fullCollection(): (() => void) | null {
    if (ReplicaMemory.collector === undefined) {
      ReplicaMemory.collector = null;
      try {
        require("node:v8").setFlagsFromString("--expose-gc");
        const gc: unknown = require("node:vm").runInNewContext("gc");
        if (typeof gc === "function") {
          const collect = gc as () => void;
          // With RPK_REPLICA_MEMORY_LOG set, the main thread reports its heap
          // after every collection: the live data of one replica
          const log = process.env["RPK_REPLICA_MEMORY_LOG"] !== undefined
            && require("node:worker_threads").isMainThread;
          ReplicaMemory.collector = log ? (): void => {
            collect();
            ReplicaMemory.reportLiveMemory();
          } : collect;
        }
      }
      catch {
        ReplicaMemory.collector = null;
      }
    }
    return ReplicaMemory.collector;
  }

  private static reportLiveMemory(): void {
    const megabytes = (bytes: number): string => (bytes / 1048576).toFixed(0);
    const heap = require("node:v8").getHeapStatistics();
    const usage = process.memoryUsage();
    process.stderr.write(
      `[replicaMemory] live heap ${megabytes(heap.used_heap_size)} MB, heap ${megabytes(heap.total_heap_size)} MB, `
      + `external ${megabytes(usage.external)} MB, array buffers ${megabytes(usage.arrayBuffers)} MB, `
      + `process rss ${megabytes(usage.rss)} MB\n`,
    );
  }

  /** The options of the Node worker of a replica */
  public static workerOptions(): NodeWorkerOptions {
    return { resourceLimits: { maxYoungGenerationSizeMb: ReplicaMemory.WORKER_YOUNG_GENERATION_MB } };
  }
}
