import type { WorkerExecutor, WorkerTransferValue } from "../../../../java/concurrent/WorkerProtocol";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import type { ShadowStatistics } from "../../common/statistics/ShadowStatistics";

/**
What the owner of a parallel Jacobi iteration sends to start one replica (see
RefinementReplicas): the command line the replica runs, its worker number, and
the shared memory of the iterations
*/
export interface RefinementReplicaRequest {
  readonly argv: string[];
  readonly worker: number;
  readonly threads: number;
  readonly control: Int32Array;
  readonly shadowCounts: Float64Array;
  readonly records: Float64Array[];
  readonly itemResults: Int32Array;
  readonly forkTable: Int32Array;
  readonly forkData: Float64Array;
  readonly pendingTasks: Int32Array;
  readonly [key: string]: WorkerTransferValue;
}

/** Creates one replica worker; the application decides how (a Node worker module) */
export type RefinementReplicaFactory = () => WorkerExecutor<RefinementReplicaRequest, null>;

/**
The threads of the parallel Jacobi iterations, and the memory they share.

TypeScript adaptation, without C++ counterpart (C++ threads share one heap and
work over the very objects of the solver). A JavaScript worker shares no
objects, so, as VITRAL's ParallelRaytracer rebuilds its scene in every worker
from a descriptor (the scene file name) instead of copying the object graph,
every worker here is a replica: it runs the same program with the same command
line (the descriptor), and so holds the same scene and Galerkin state, bit for
bit, with all its writes discarded by the application. The main thread is
worker 0. In every parallel Jacobi iteration each replica refines the items it
takes and records what it did in its record (see RefinementRecord); then every
replica commits all the records in the serial order, so all of them end the
iteration with the state of the serial refinement and go on in lockstep.

The application provides the workers (setReplicaFactory), as it provides the
ParallelRaytracer's ones: VITRAL's NodeWorkerExecutor over a worker module that
calls joinAsReplica and then runs the program. Without a factory, refinement
runs in one thread.

A solver iteration is synchronous (it can not wait for a Promise as
VITRAL's ParallelRaytracer does), so the replicas synchronize over
SharedArrayBuffers with Atomics: a lock and a condition for the forked tasks
(RefinementScheduler), barriers between the phases of an iteration, the record
of each replica, the results of the tasks and the forked tasks themselves.
*/
export class RefinementReplicas {
  // Words of the control array
  public static readonly LOCK = 0;
  public static readonly CONDITION = 1;
  public static readonly NEXT_ITEM = 2;
  public static readonly PENDING_COUNT = 3;
  public static readonly RUNNING = 4;
  public static readonly FAILED = 5;
  public static readonly FORK_COUNT = 6;
  public static readonly FORK_DATA_SIZE = 7;
  private static readonly BARRIER_COUNT = 8;
  private static readonly BARRIER_GENERATION = 9;
  private static readonly HEADER_SIZE = 16;
  // Words of each worker, after the header
  private static readonly RECORD_SIZE = 0;
  private static readonly CHECK_ITEMS = 1;
  private static readonly CHECK_ELEMENTS = 2;
  private static readonly CHECK_INTERACTIONS = 3;
  private static readonly WORDS_PER_WORKER = 4;

  private static readonly RECORD_MAXIMUM_BYTES = 2 ** 33;
  private static readonly ITEM_RESULTS_MAXIMUM_BYTES = 2 ** 32;
  private static readonly FORK_TABLE_MAXIMUM_BYTES = 2 ** 31;
  private static readonly FORK_DATA_MAXIMUM_BYTES = 2 ** 32;
  private static readonly PENDING_TASKS_MAXIMUM_BYTES = 2 ** 30;
  private static readonly WAIT_MILLISECONDS = 1000;

  private static replicaFactory: RefinementReplicaFactory | null = null;
  private static descriptor: string[] = [];
  private static current: RefinementReplicas | null = null;
  private static missingFactoryReported = false;

  private numberOfThreads: number; // As asked for by the iterations
  private readonly worker: number;
  private readonly control: Int32Array;
  private readonly shadowCounts: Float64Array;
  private readonly records: Float64Array[];
  private readonly itemResults: Int32Array;
  private readonly forkTable: Int32Array;
  private readonly forkData: Float64Array;
  private readonly pendingTasks: Int32Array;
  private readonly executors: WorkerExecutor<RefinementReplicaRequest, null>[];

  private constructor(request: RefinementReplicaRequest) {
    this.numberOfThreads = request.threads;
    this.worker = request.worker;
    this.control = request.control;
    this.shadowCounts = request.shadowCounts;
    // Views that follow the length of their growable buffers
    this.records = request.records.map((view: Float64Array): Float64Array => new Float64Array(view.buffer));
    this.itemResults = new Int32Array(request.itemResults.buffer);
    this.forkTable = new Int32Array(request.forkTable.buffer);
    this.forkData = new Float64Array(request.forkData.buffer);
    this.pendingTasks = new Int32Array(request.pendingTasks.buffer);
    this.executors = [];
  }

  /**
  Sets how the workers of the parallel Jacobi iterations are created, and the
  command line they run (normally the one of this program)
  */
  public static setReplicaFactory(factory: RefinementReplicaFactory | null, descriptor: string[]): void {
    RefinementReplicas.replicaFactory = factory;
    RefinementReplicas.descriptor = descriptor.slice();
  }

  /**
  Called by the worker module of a replica, before it runs the program
  */
  public static joinAsReplica(request: RefinementReplicaRequest): void {
    const replicas = new RefinementReplicas(request);
    RefinementReplicas.current = replicas;
    // A replica that ends badly (i.e. a fatal error) must not leave the
    // others waiting for it
    process.on("exit", (code: number): void => {
      if (code !== 0) {
        replicas.fail();
      }
    });
  }

  /** The replica of this worker failed (called by its worker module) */
  public static failReplica(): void {
    if (RefinementReplicas.current !== null) {
      RefinementReplicas.current.fail();
    }
  }

  /** True in the worker of a replica */
  public static isReplica(): boolean {
    return RefinementReplicas.current !== null && RefinementReplicas.current.worker !== 0;
  }

  /**
  The replicas of a parallel Jacobi iteration with the given number of
  threads. The first call of the main thread starts the workers (they run the
  program up to this iteration and then wait here)
  */
  public static forThreads(numberOfThreads: number): RefinementReplicas {
    const current = RefinementReplicas.current;
    if (current !== null) {
      if (current.numberOfThreads !== numberOfThreads) {
        VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "RefinementReplicas::forThreads", "The number of parallel Jacobi threads changed");
      }
      return current;
    }

    let threads = numberOfThreads;
    if (threads > 1 && RefinementReplicas.replicaFactory === null) {
      if (!RefinementReplicas.missingFactoryReported) {
        RefinementReplicas.missingFactoryReported = true;
        VsdkLogger.reportMessage(null, VsdkLogger.WARNING, "RefinementReplicas::forThreads", "No parallel Jacobi workers can be created here: refining in one thread");
      }
      threads = 1;
    }

    const buffer = (bytes: number, maximumBytes: number): SharedArrayBuffer =>
      new (SharedArrayBuffer as unknown as new (length: number, options: { maxByteLength: number }) => SharedArrayBuffer)(
        bytes,
        { maxByteLength: maximumBytes },
      );
    const records: Float64Array[] = [];
    for (let w = 0; w < threads; w++) {
      records.push(new Float64Array(buffer(8 * 1024, RefinementReplicas.RECORD_MAXIMUM_BYTES)));
    }
    const request: RefinementReplicaRequest = {
      argv: RefinementReplicas.descriptor,
      worker: 0,
      threads,
      control: new Int32Array(new SharedArrayBuffer(
        4 * (RefinementReplicas.HEADER_SIZE + RefinementReplicas.WORDS_PER_WORKER * threads),
      )),
      shadowCounts: new Float64Array(new SharedArrayBuffer(8 * 2 * threads)),
      records,
      itemResults: new Int32Array(buffer(4 * 1024, RefinementReplicas.ITEM_RESULTS_MAXIMUM_BYTES)),
      forkTable: new Int32Array(buffer(4 * 1024, RefinementReplicas.FORK_TABLE_MAXIMUM_BYTES)),
      forkData: new Float64Array(buffer(8 * 1024, RefinementReplicas.FORK_DATA_MAXIMUM_BYTES)),
      pendingTasks: new Int32Array(buffer(4 * 1024, RefinementReplicas.PENDING_TASKS_MAXIMUM_BYTES)),
    };
    const replicas = new RefinementReplicas(request);
    RefinementReplicas.current = replicas;
    for (let w = 1; w < threads; w++) {
      const executor = RefinementReplicas.replicaFactory!();
      replicas.executors.push(executor);
      // Never answers while the program runs: the replica ends with it
      executor.execute({ ...request, worker: w }).catch((): void => {
        replicas.fail();
      });
    }
    replicas.numberOfThreads = numberOfThreads;
    return replicas;
  }

  /** Number of threads refining (the main one and the replica workers) */
  public getNumberOfWorkers(): number {
    return this.records.length;
  }

  /** The worker number of this thread: 0 for the main thread */
  public getWorker(): number {
    return this.worker;
  }

  public getControl(): Int32Array {
    return this.control;
  }

  public getRecordEvents(worker: number): Float64Array {
    return this.records[worker]!;
  }

  public getItemResults(): Int32Array {
    return this.itemResults;
  }

  public getForkTable(): Int32Array {
    return this.forkTable;
  }

  public getForkData(): Float64Array {
    return this.forkData;
  }

  public getPendingTasks(): Int32Array {
    return this.pendingTasks;
  }

  /**
  Makes the growable buffer of the view hold at least the given number of
  elements (only the thread that writes it grows it)
  */
  public static ensureLength(view: Float64Array | Int32Array, length: number): void {
    if (view.length >= length) {
      return;
    }
    const buffer = view.buffer as unknown as { byteLength: number; maxByteLength: number; grow(bytes: number): void };
    const bytes = length * view.BYTES_PER_ELEMENT;
    if (bytes > buffer.maxByteLength) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "RefinementReplicas::ensureLength", "Parallel Jacobi shared memory exhausted");
    }
    buffer.grow(Math.min(buffer.maxByteLength, Math.max(bytes, 2 * buffer.byteLength)));
  }

  public lock(): void {
    const control = this.control;
    while (Atomics.compareExchange(control, RefinementReplicas.LOCK, 0, 1) !== 0) {
      Atomics.wait(control, RefinementReplicas.LOCK, 1, RefinementReplicas.WAIT_MILLISECONDS);
    }
  }

  public unlock(): void {
    Atomics.store(this.control, RefinementReplicas.LOCK, 0);
    Atomics.notify(this.control, RefinementReplicas.LOCK, 1);
  }

  /** Condition.await: called with the lock, which it releases while waiting */
  public await(): void {
    const sequence = Atomics.load(this.control, RefinementReplicas.CONDITION);
    this.unlock();
    Atomics.wait(this.control, RefinementReplicas.CONDITION, sequence, RefinementReplicas.WAIT_MILLISECONDS);
    this.lock();
  }

  public signal(): void {
    Atomics.add(this.control, RefinementReplicas.CONDITION, 1);
    Atomics.notify(this.control, RefinementReplicas.CONDITION, 1);
  }

  public signalAll(): void {
    Atomics.add(this.control, RefinementReplicas.CONDITION, 1);
    Atomics.notify(this.control, RefinementReplicas.CONDITION);
  }

  /** A worker failed: the others stop */
  public fail(): void {
    Atomics.store(this.control, RefinementReplicas.FAILED, 1);
    Atomics.notify(this.control, RefinementReplicas.CONDITION);
    Atomics.notify(this.control, RefinementReplicas.BARRIER_GENERATION);
    Atomics.notify(this.control, RefinementReplicas.LOCK);
  }

  public hasFailed(): boolean {
    return Atomics.load(this.control, RefinementReplicas.FAILED) !== 0;
  }

  /**
  Waits until all the workers arrive. The last one to arrive runs lastAction
  (while all the others wait) before letting them go on
  */
  public arrive(lastAction: (() => void) | null): void {
    const control = this.control;
    const generation = Atomics.load(control, RefinementReplicas.BARRIER_GENERATION);
    if (Atomics.add(control, RefinementReplicas.BARRIER_COUNT, 1) === this.getNumberOfWorkers() - 1) {
      Atomics.store(control, RefinementReplicas.BARRIER_COUNT, 0);
      if (lastAction !== null) {
        lastAction();
      }
      Atomics.add(control, RefinementReplicas.BARRIER_GENERATION, 1);
      Atomics.notify(control, RefinementReplicas.BARRIER_GENERATION);
      return;
    }
    while (Atomics.load(control, RefinementReplicas.BARRIER_GENERATION) === generation) {
      if (this.hasFailed()) {
        VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "RefinementReplicas::arrive", "A refinement worker failed");
      }
      Atomics.wait(control, RefinementReplicas.BARRIER_GENERATION, generation, RefinementReplicas.WAIT_MILLISECONDS);
    }
  }

  /**
  Start of a parallel Jacobi iteration: waits for all the replicas, and checks
  that all of them are at the same point (same items, elements and
  interactions). The last one to arrive prepares the shared state for the
  given number of items
  */
  public startIteration(numberOfItems: number, numberOfElements: number, numberOfInteractions: number): void {
    const control = this.control;
    const base = RefinementReplicas.HEADER_SIZE + RefinementReplicas.WORDS_PER_WORKER * this.worker;
    control[base + RefinementReplicas.CHECK_ITEMS] = numberOfItems;
    control[base + RefinementReplicas.CHECK_ELEMENTS] = numberOfElements;
    control[base + RefinementReplicas.CHECK_INTERACTIONS] = numberOfInteractions;
    this.arrive((): void => {
      RefinementReplicas.ensureLength(this.itemResults, 4 * numberOfItems);
      Atomics.store(control, RefinementReplicas.NEXT_ITEM, 0);
      Atomics.store(control, RefinementReplicas.PENDING_COUNT, 0);
      Atomics.store(control, RefinementReplicas.RUNNING, 0);
      Atomics.store(control, RefinementReplicas.FORK_COUNT, 0);
      Atomics.store(control, RefinementReplicas.FORK_DATA_SIZE, 0);
    });
    for (let w = 0; w < this.getNumberOfWorkers(); w++) {
      const other = RefinementReplicas.HEADER_SIZE + RefinementReplicas.WORDS_PER_WORKER * w;
      if (control[other + RefinementReplicas.CHECK_ITEMS] !== numberOfItems
        || control[other + RefinementReplicas.CHECK_ELEMENTS] !== numberOfElements
        || control[other + RefinementReplicas.CHECK_INTERACTIONS] !== numberOfInteractions) {
        VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "RefinementReplicas::startIteration", "Parallel Jacobi replicas out of step");
      }
    }
  }

  /**
  End of the refinement of an iteration: publishes the size of this worker's
  record and its shadow ray counters, and waits for all the workers
  */
  public endRefinement(recordSize: number, shadowStatistics: ShadowStatistics): void {
    const base = RefinementReplicas.HEADER_SIZE + RefinementReplicas.WORDS_PER_WORKER * this.worker;
    this.control[base + RefinementReplicas.RECORD_SIZE] = recordSize;
    this.shadowCounts[2 * this.worker] = shadowStatistics.numberOfShadowRays;
    this.shadowCounts[2 * this.worker + 1] = shadowStatistics.numberOfShadowCacheHits;
    this.arrive(null);
  }

  /** After endRefinement: the shadow ray counters of all the workers, in their order */
  public addShadowStatistics(shadowStatistics: ShadowStatistics): void {
    for (let w = 0; w < this.getNumberOfWorkers(); w++) {
      shadowStatistics.numberOfShadowRays += this.shadowCounts[2 * w]!;
      shadowStatistics.numberOfShadowCacheHits += this.shadowCounts[2 * w + 1]!;
    }
  }
}
