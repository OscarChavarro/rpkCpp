import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import type { Scene } from "../../scene/Scene";
import type { Geometry } from "../../skin/Geometry";
import { GalerkinElement } from "../GalerkinElement";
import { Interaction } from "../Interaction";
import type { FormFactorWorkspace } from "./FormFactorWorkspace";
import type { ScratchVisibilityWorkspace } from "./ScratchVisibilityWorkspace";
import { RefinementReplicas } from "./RefinementReplicas";

/**
What the refinement of one interaction did to shared state, recorded by a
parallel Jacobi worker instead of being done at once, so that it can be done
later in the serial order:
- SUBDIVISION: the element was regularly subdivided (or already was); its new
  children get their ids when the event is committed
- LINK: a refined interaction, created by the worker but not yet counted, to
  store with its receiver and transport light over
- FORK: the refinement of a sub-interaction left to another task (by its index
  in the scheduler); the events of that task go here

TypeScript adaptation: the workers are replicas of the program that share no
objects (see RefinementReplicas), so events are numbers in the shared record of
their worker, and every replica rebuilds them in its own heap (read). An
element is named by the id of the nearest of its ancestors (or itself) that
existed when the iteration started (new children have id 0 until the commit)
and the child numbers of the path from it; a link by its two elements and its
numbers (it is the interaction Interaction.interactionDuplicate would create,
but not counted). The C++ NODES_TOO_CLOSE event is not needed: the TypeScript
form factor code does not report that warning.
*/
export class RefinementEvent {
  public static readonly SUBDIVISION = 0;
  public static readonly LINK = 2;
  public static readonly FORK = 3;

  private kind = RefinementEvent.LINK;
  private receiver: GalerkinElement | null = null;
  private link: Interaction | null = null;
  private task = 0;

  public getKind(): number {
    return this.kind;
  }

  /** The subdivided element */
  public getReceiver(): GalerkinElement {
    return this.receiver!;
  }

  public getLink(): Interaction {
    return this.link!;
  }

  public getTask(): number {
    return this.task;
  }

  /**
  Writes the reference of an element at the end of the numbers
  */
  public static writeElement(numbers: number[], element: GalerkinElement): void {
    const start = numbers.length;
    let base: GalerkinElement = element;
    let depth = 0;
    numbers.push(0, 0);
    while (base.id === 0) {
      numbers.push(base.childNumber);
      depth++;
      base = base.parent as GalerkinElement;
    }
    numbers[start] = base.id;
    numbers[start + 1] = depth;
    // Child numbers from the base element down
    for (let i = start + 2, j = numbers.length - 1; i < j; i++, j--) {
      const t = numbers[i]!;
      numbers[i] = numbers[j]!;
      numbers[j] = t;
    }
  }

  /**
  The element of the reference at offset, subdivided along the path in this
  replica where it was not yet
  @param position in: offset of the reference; out: offset after it
  */
  public static readElement(
    numbers: Float64Array,
    position: number[],
    elements: Array<GalerkinElement | undefined>,
  ): GalerkinElement {
    let offset = position[0]!;
    let element = elements[numbers[offset]!];
    if (element === undefined) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "RefinementEvent::readElement", "Unknown element in a parallel Jacobi record");
    }
    const depth = numbers[offset + 1]!;
    offset += 2;
    for (let k = 0; k < depth; k++) {
      RefinementWorkspace.subdivideUncommitted(element!);
      element = element!.regularSubElements![numbers[offset++]!] as GalerkinElement;
    }
    position[0] = offset;
    return element!;
  }

  /**
  Reads the event at the offset (creating, in this replica, the elements and
  the link it names)
  @return the offset of the next event
  */
  public read(numbers: Float64Array, offset: number, elements: Array<GalerkinElement | undefined>): number {
    const position = [offset + 1];
    this.kind = numbers[offset]!;
    switch (this.kind) {
      case RefinementEvent.SUBDIVISION:
        this.receiver = RefinementEvent.readElement(numbers, position, elements);
        break;
      case RefinementEvent.LINK: {
        const link = new Interaction(); // Not counted
        link.receiverElement = RefinementEvent.readElement(numbers, position, elements);
        link.sourceElement = RefinementEvent.readElement(numbers, position, elements);
        let p = position[0]!;
        link.numberOfBasisFunctionsOnReceiver = numbers[p++]!;
        link.numberOfBasisFunctionsOnSource = numbers[p++]!;
        link.numberOfReceiverCubaturePositions = numbers[p++]!;
        link.visibility = numbers[p++]!;
        link.deltaK = numbers[p++]!;
        const n = numbers[p++]!;
        link.K = new Array<number>(n);
        for (let i = 0; i < n; i++) {
          link.K[i] = numbers[p++]!;
        }
        if ((link.numberOfReceiverCubaturePositions & 0xff) > 1) {
          VsdkLogger.reportMessage(link, VsdkLogger.FATAL_ERROR, "interactionCreate", "Not yet implemented for higher order approximations");
        }
        position[0] = p;
        this.link = link;
        break;
      }
      case RefinementEvent.FORK:
        this.task = numbers[position[0]!]!;
        position[0] = position[0]! + 1;
        break;
      default:
        VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "RefinementEvent::read", "Corrupt parallel Jacobi record");
    }
    return position[0]!;
  }
}

/**
The events of the interactions refined by one worker, in the order in which
the worker refined them (TypeScript: in the shared buffer of the worker; an
event is named by its offset there)
*/
export class RefinementRecord {
  private readonly events: Float64Array;
  private size = 0;
  private readonly scratch: number[] = [];

  public constructor(events: Float64Array) {
    this.events = events;
  }

  public getSize(): number {
    return this.size;
  }

  public clear(): void {
    this.size = 0;
  }

  private append(numbers: number[]): void {
    RefinementReplicas.ensureLength(this.events, this.size + numbers.length);
    const events = this.events;
    let s = this.size;
    for (let i = 0; i < numbers.length; i++) {
      events[s++] = numbers[i]!;
    }
    this.size = s;
    numbers.length = 0;
  }

  public addSubdivision(element: GalerkinElement): void {
    const numbers = this.scratch;
    numbers.push(RefinementEvent.SUBDIVISION);
    RefinementEvent.writeElement(numbers, element);
    this.append(numbers);
  }

  /**
  The link interactionDuplicate would make of the interaction
  */
  public addLink(interaction: Interaction): void {
    const numbers = this.scratch;
    numbers.push(RefinementEvent.LINK);
    RefinementEvent.writeElement(numbers, interaction.receiverElement);
    RefinementEvent.writeElement(numbers, interaction.sourceElement);
    numbers.push(
      interaction.numberOfBasisFunctionsOnReceiver ?? 0,
      interaction.numberOfBasisFunctionsOnSource ?? 0,
      interaction.numberOfReceiverCubaturePositions ?? 0,
      interaction.visibility ?? 0,
      interaction.deltaK ?? 0.0,
    );
    let n = (interaction.numberOfBasisFunctionsOnReceiver & 0xff) * (interaction.numberOfBasisFunctionsOnSource & 0xff);
    if (n <= 0) {
      n = 1;
    }
    numbers.push(n);
    const K = interaction.K;
    for (let i = 0; i < n; i++) {
      numbers.push(K !== null && i < K.length ? (K[i] ?? 0.0) : 0.0);
    }
    this.append(numbers);
  }

  public addFork(task: number): void {
    const numbers = this.scratch;
    numbers.push(RefinementEvent.FORK, task);
    this.append(numbers);
  }
}

/**
A task of a parallel Jacobi iteration: one interaction of an element list
(refined as refineInteraction does), or a sub-interaction between two
elements forked by a subdivision (refined as refineSubInteraction does). Once
run by a worker, the result: whether a top-level interaction was refined
(replaced by the links of its events) and the range [firstEvent, endEvent) of
its events in the record of that worker
(TypeScript: the result is kept in the shared memory of the scheduler, which
fills it in when it hands the task out for the commit)
*/
export class RefinementTask {
  private readonly interaction: unknown;
  private readonly receiver: GalerkinElement | null;
  private readonly source: GalerkinElement | null;
  private readonly candidatesList: Geometry[] | null;
  private readonly index: number;
  private worker = -1;
  private firstEvent = 0;
  private endEvent = 0;
  private refined = false;

  /**
  @param interaction an entry of an element list (an Interaction, or kept as it is)
  @param receiver, source, candidatesList those of a sub-interaction
  @param index of the item or of the forked task
  */
  public constructor(
    interaction: unknown,
    receiver: GalerkinElement | null,
    source: GalerkinElement | null,
    candidatesList: Geometry[] | null,
    index: number,
  ) {
    this.interaction = interaction;
    this.receiver = receiver;
    this.source = source;
    this.candidatesList = candidatesList;
    this.index = index;
  }

  /** null for a sub-interaction */
  public getInteraction(): unknown {
    return this.interaction;
  }

  public getReceiver(): GalerkinElement {
    return this.receiver!;
  }

  public getSource(): GalerkinElement {
    return this.source!;
  }

  public getCandidatesList(): Geometry[] | null {
    return this.candidatesList;
  }

  public getIndex(): number {
    return this.index;
  }

  public getWorker(): number {
    return this.worker;
  }

  public getFirstEvent(): number {
    return this.firstEvent;
  }

  public getEndEvent(): number {
    return this.endEvent;
  }

  public isRefined(): boolean {
    return this.refined;
  }

  public setResult(inWorker: number, inFirstEvent: number, inEndEvent: number, inRefined: boolean): void {
    this.worker = inWorker;
    this.firstEvent = inFirstEvent;
    this.endEvent = inEndEvent;
    this.refined = inRefined;
  }
}

/**
The tasks of a parallel Jacobi iteration. The items (interactions of the
element lists, fixed before the workers start) are handed out in chunks
through an atomic counter, without locking; the sub-interactions forked by
subdivisions go to a locked stack. Subdivisions fork only once the items are
all handed out, while few forked tasks wait, so that the many cheap items of
the late iterations cost no lock, and the few expensive ones of the early
iterations are shared. Workers end when no item is left, no forked task waits
and none runs (a running task may fork more).
TypeScript: the counters, the lock, the condition, the results and the forked
tasks (their elements as RefinementEvent references) are in the shared memory
of the replicas; every replica has its own copy of the items, in the same order.
*/
export class RefinementScheduler {
  private static readonly NO_CANDIDATES = 0;
  private static readonly CLUSTERED_GEOMETRY = 1;
  private static readonly SCENE_GEOMETRY = 2;

  private readonly replicas: RefinementReplicas;
  private readonly control: Int32Array;
  private readonly items: RefinementTask[];
  private readonly numberOfItems: number;
  private readonly chunk: number;
  private readonly forkThreshold: number;
  private readonly scene: Scene;
  private readonly elements: Array<GalerkinElement | undefined>;
  private readonly scratch: number[] = [];

  /**
  @param inItems the items, as in every replica
  @param inChunk number of items handed out at once
  @param inForkThreshold sub-interactions are forked while fewer tasks wait
  @param inElements elements by id, those that existed when the iteration started
  */
  public constructor(
    inReplicas: RefinementReplicas,
    inItems: RefinementTask[],
    inChunk: number,
    inForkThreshold: number,
    inScene: Scene,
    inElements: Array<GalerkinElement | undefined>,
  ) {
    this.replicas = inReplicas;
    this.control = inReplicas.getControl();
    this.items = inItems;
    this.numberOfItems = inItems.length;
    this.chunk = inChunk;
    this.forkThreshold = inForkThreshold;
    this.scene = inScene;
    this.elements = inElements;
  }

  /**
  Hands out the next chunk of items, [range[0], range[1])
  @return false when all items were handed out
  */
  public takeItems(range: number[]): boolean {
    if (Atomics.load(this.control, RefinementReplicas.NEXT_ITEM) >= this.numberOfItems) {
      return false;
    }
    const start = Atomics.add(this.control, RefinementReplicas.NEXT_ITEM, this.chunk);
    if (start >= this.numberOfItems) {
      return false;
    }
    range[0] = start;
    range[1] = start + this.chunk < this.numberOfItems ? start + this.chunk : this.numberOfItems;
    return true;
  }

  public getItem(index: number): RefinementTask {
    return this.items[index]!;
  }

  /** After the workers ended: the item, with its result */
  public getItemWithResult(index: number): RefinementTask {
    const task = this.items[index]!;
    const results = this.replicas.getItemResults();
    task.setResult(results[4 * index]!, results[4 * index + 1]!, results[4 * index + 2]!, results[4 * index + 3]! !== 0);
    return task;
  }

  /** Result of a task run by the calling worker */
  public setResult(task: RefinementTask, worker: number, firstEvent: number, endEvent: number, refined: boolean): void {
    const results = task.getInteraction() === null ? this.replicas.getForkTable() : this.replicas.getItemResults();
    const i = 4 * task.getIndex();
    results[i] = worker;
    results[i + 1] = firstEvent;
    results[i + 2] = endEvent;
    results[i + 3] = refined ? 1 : 0;
  }

  public wantsMoreTasks(): boolean {
    return Atomics.load(this.control, RefinementReplicas.NEXT_ITEM) >= this.numberOfItems
      && Atomics.load(this.control, RefinementReplicas.PENDING_COUNT) < this.forkThreshold;
  }

  private candidatesKind(candidatesList: Geometry[] | null): number {
    if (candidatesList === null) {
      return RefinementScheduler.NO_CANDIDATES;
    }
    if (candidatesList === this.scene.clusteredGeometryList) {
      return RefinementScheduler.CLUSTERED_GEOMETRY;
    }
    if (candidatesList === this.scene.geometryList) {
      return RefinementScheduler.SCENE_GEOMETRY;
    }
    VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "RefinementScheduler::fork", "Culled candidate lists can not be forked");
    return RefinementScheduler.NO_CANDIDATES;
  }

  private candidatesOfKind(kind: number): Geometry[] | null {
    switch (kind) {
      case RefinementScheduler.CLUSTERED_GEOMETRY:
        return this.scene.clusteredGeometryList;
      case RefinementScheduler.SCENE_GEOMETRY:
        return this.scene.geometryList;
      default:
        return null;
    }
  }

  /**
  @return the index of the new forked task
  */
  public fork(receiver: GalerkinElement, source: GalerkinElement, candidatesList: Geometry[] | null): number {
    const numbers = this.scratch;
    numbers.push(this.candidatesKind(candidatesList));
    RefinementEvent.writeElement(numbers, receiver);
    RefinementEvent.writeElement(numbers, source);

    const replicas = this.replicas;
    const control = this.control;
    replicas.lock();
    const index = control[RefinementReplicas.FORK_COUNT]!;
    const dataOffset = control[RefinementReplicas.FORK_DATA_SIZE]!;
    const forkData = replicas.getForkData();
    RefinementReplicas.ensureLength(forkData, dataOffset + numbers.length);
    for (let i = 0; i < numbers.length; i++) {
      forkData[dataOffset + i] = numbers[i]!;
    }
    control[RefinementReplicas.FORK_DATA_SIZE] = dataOffset + numbers.length;
    numbers.length = 0;
    const forkTable = replicas.getForkTable();
    RefinementReplicas.ensureLength(forkTable, 4 * (index + 1));
    forkTable[4 * index] = -1;
    forkTable[4 * index + 1] = dataOffset; // Until the task is run
    control[RefinementReplicas.FORK_COUNT] = index + 1;
    const pendingCount = control[RefinementReplicas.PENDING_COUNT]!;
    const pendingTasks = replicas.getPendingTasks();
    RefinementReplicas.ensureLength(pendingTasks, pendingCount + 1);
    pendingTasks[pendingCount] = index;
    Atomics.add(control, RefinementReplicas.PENDING_COUNT, 1);
    replicas.signal();
    replicas.unlock();
    return index;
  }

  /**
  Takes a forked task, waiting for one while other tasks run (call it once
  all items were handed out, and done() after running the task)
  @return the task, or null when all are done (or a worker failed)
  */
  public takeForked(): RefinementTask | null {
    const replicas = this.replicas;
    const control = this.control;
    replicas.lock();
    while (control[RefinementReplicas.PENDING_COUNT] === 0 && control[RefinementReplicas.RUNNING]! > 0 && !replicas.hasFailed()) {
      replicas.await();
    }
    let index = -1;
    let dataOffset = 0;
    if (control[RefinementReplicas.PENDING_COUNT]! > 0 && !replicas.hasFailed()) {
      const pendingCount = control[RefinementReplicas.PENDING_COUNT]! - 1;
      index = replicas.getPendingTasks()[pendingCount]!;
      dataOffset = replicas.getForkTable()[4 * index + 1]!;
      Atomics.sub(control, RefinementReplicas.PENDING_COUNT, 1);
      control[RefinementReplicas.RUNNING] = control[RefinementReplicas.RUNNING]! + 1;
    }
    else {
      replicas.signalAll();
    }
    replicas.unlock();
    if (index < 0) {
      return null;
    }

    // The elements of the task, in this replica
    const forkData = replicas.getForkData();
    const candidatesList = this.candidatesOfKind(forkData[dataOffset]!);
    const position = [dataOffset + 1];
    const receiver = RefinementEvent.readElement(forkData, position, this.elements);
    const source = RefinementEvent.readElement(forkData, position, this.elements);
    return new RefinementTask(null, receiver, source, candidatesList, index);
  }

  /** Called by a worker before running a chunk of items, which may fork */
  public startItems(): void {
    this.replicas.lock();
    this.control[RefinementReplicas.RUNNING] = this.control[RefinementReplicas.RUNNING]! + 1;
    this.replicas.unlock();
  }

  /** The forked task or chunk of items of the calling worker is done */
  public done(): void {
    const replicas = this.replicas;
    const control = this.control;
    replicas.lock();
    control[RefinementReplicas.RUNNING] = control[RefinementReplicas.RUNNING]! - 1;
    if (control[RefinementReplicas.RUNNING] === 0 && control[RefinementReplicas.PENDING_COUNT] === 0) {
      replicas.signalAll();
    }
    replicas.unlock();
  }

  /** The calling worker failed: the others stop */
  public fail(): void {
    const replicas = this.replicas;
    replicas.lock();
    this.control[RefinementReplicas.RUNNING] = this.control[RefinementReplicas.RUNNING]! - 1;
    Atomics.store(this.control, RefinementReplicas.NEXT_ITEM, this.numberOfItems);
    replicas.fail();
    replicas.signalAll();
    replicas.unlock();
  }

  public hasFailed(): boolean {
    return this.replicas.hasFailed();
  }

  /** After the workers ended (only its result is known) */
  public getForked(index: number): RefinementTask {
    const forkTable = this.replicas.getForkTable();
    const task = new RefinementTask(null, null, null, null, index);
    task.setResult(forkTable[4 * index]!, forkTable[4 * index + 1]!, forkTable[4 * index + 2]!, false);
    return task;
  }
}

/**
An element whose interactions are refined in a parallel Jacobi iteration: its
tasks are numberOfItems consecutive ones from firstItem, in the order of its
interaction list when the iteration starts
*/
export class RefinementGroup {
  private readonly element: GalerkinElement;
  private readonly firstItem: number;
  private readonly numberOfItems: number;

  public constructor(inElement: GalerkinElement, inFirstItem: number, inNumberOfItems: number) {
    this.element = inElement;
    this.firstItem = inFirstItem;
    this.numberOfItems = inNumberOfItems;
  }

  public getElement(): GalerkinElement {
    return this.element;
  }

  public getFirstItem(): number {
    return this.firstItem;
  }

  public getNumberOfItems(): number {
    return this.numberOfItems;
  }
}

/**
Mutable scratch of one thread of hierarchical refinement. The serial solver
uses the scene's query workspace, the solver's form factor and scratch
visibility workspaces and no record: light transport and interaction storage
are done at once. A parallel Jacobi worker has its own form factor and scratch
visibility (Z-visibility clustering only) scratch, a record receiving what is
to be done later in the serial order and the scheduler of the tasks.
Nothing is owned.
TypeScript: there is no subdivision lock (each replica subdivides its own
elements), and a worker uses the query workspace of its replica's scene (no
other thread uses it).
*/
export class RefinementWorkspace {
  private readonly sceneQueryWorkspace: SceneQueryWorkspace | null;
  private readonly formFactorWorkspace: FormFactorWorkspace;
  private readonly scratchVisibilityWorkspace: ScratchVisibilityWorkspace | null;
  private readonly coefficientsPool: number[][];
  private readonly record: RefinementRecord | null;
  private readonly scheduler: RefinementScheduler | null;

  public constructor(
    inSceneQueryWorkspace: SceneQueryWorkspace | null,
    inFormFactorWorkspace: FormFactorWorkspace,
    inScratchVisibilityWorkspace: ScratchVisibilityWorkspace | null,
    inCoefficientsPool: number[][],
    inRecord: RefinementRecord | null,
    inScheduler: RefinementScheduler | null,
  ) {
    this.sceneQueryWorkspace = inSceneQueryWorkspace;
    this.formFactorWorkspace = inFormFactorWorkspace;
    this.scratchVisibilityWorkspace = inScratchVisibilityWorkspace;
    this.coefficientsPool = inCoefficientsPool;
    this.record = inRecord;
    this.scheduler = inScheduler;
  }

  public getSceneQueryWorkspace(): SceneQueryWorkspace | null {
    return this.sceneQueryWorkspace;
  }

  public getFormFactorWorkspace(): FormFactorWorkspace {
    return this.formFactorWorkspace;
  }

  /** null unless the clustering strategy is Z-visibility */
  public getScratchVisibilityWorkspace(): ScratchVisibilityWorkspace | null {
    return this.scratchVisibilityWorkspace;
  }

  public getCoefficientsPool(): number[][] {
    return this.coefficientsPool;
  }

  /** null for the serial solver */
  public getRecord(): RefinementRecord | null {
    return this.record;
  }

  /** null for the serial solver */
  public getScheduler(): RefinementScheduler | null {
    return this.scheduler;
  }

  /**
  Regular subdivision of an element during a parallel Jacobi iteration: the
  new children have id 0 until their subdivision is committed
  */
  public static subdivideUncommitted(element: GalerkinElement): void {
    if (element.regularSubElements !== null) {
      return;
    }
    element.regularSubDivide();
    const children = element.regularSubElements as Array<GalerkinElement | null> | null;
    if (children === null) {
      return;
    }
    for (let i = 0; i < 4; i++) {
      const child = children[i];
      if (child !== null && child !== undefined) {
        child.id = 0;
      }
    }
  }

  /**
  The elements that exist now, by id
  */
  public static committedElements(parentElement: GalerkinElement): Array<GalerkinElement | undefined> {
    const elements: Array<GalerkinElement | undefined> = new Array<GalerkinElement | undefined>(GalerkinElement.getNumberOfElements() + 1);
    const pending: GalerkinElement[] = [parentElement];
    while (pending.length > 0) {
      const element = pending.pop()!;
      elements[element.id] = element;
      if (element.irregularSubElements !== null) {
        for (const child of element.irregularSubElements) {
          if (child instanceof GalerkinElement) {
            pending.push(child);
          }
        }
      }
      if (element.regularSubElements !== null) {
        for (const child of element.regularSubElements) {
          if (child instanceof GalerkinElement) {
            pending.push(child);
          }
        }
      }
    }
    return elements;
  }
}
