package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;
import vsdk.toolkit.common.memoryManagement.MemoryPool;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.skin.RpkGeometry;

/*
The classes of the C++ header RefinementWorkspace.h, kept in one file as
there: RefinementWorkspace, public in the package, and its helpers
RefinementEvent, RefinementRecord, RefinementTask, RefinementScheduler and
RefinementGroup, package private.
*/

/**
What the refinement of one interaction did to shared state, recorded by a
parallel Jacobi worker instead of being done at once, so that it can be done
later in the serial order:
- SUBDIVISION: the element was regularly subdivided (or already was); its new
  children get their ids when the event is committed
- NODES_TOO_CLOSE: the "nodes too close" warning for the receiver and source
- LINK: a refined interaction, created by the worker but not yet counted, to
  store with its receiver and transport light over
- FORK: the refinement of a sub-interaction left to another task (by its index
  in the scheduler); the events of that task go here
*/
final class RefinementEvent {
    enum Kind {
        SUBDIVISION,
        NODES_TOO_CLOSE,
        LINK,
        FORK
    }

    private final Kind kind;
    private final GalerkinElement receiver;
    private final GalerkinElement source;
    private final Interaction link;
    private final long task;

    private RefinementEvent(
        Kind inKind,
        GalerkinElement inReceiver,
        GalerkinElement inSource,
        Interaction inLink,
        long inTask)
    {
        kind = inKind;
        receiver = inReceiver;
        source = inSource;
        link = inLink;
        task = inTask;
    }

    static RefinementEvent subdivision(GalerkinElement element) {
        return new RefinementEvent(Kind.SUBDIVISION, element, null, null, 0);
    }

    static RefinementEvent nodesTooClose(GalerkinElement receiver, GalerkinElement source) {
        return new RefinementEvent(Kind.NODES_TOO_CLOSE, receiver, source, null, 0);
    }

    static RefinementEvent newLink(Interaction link) {
        return new RefinementEvent(Kind.LINK, null, null, link, 0);
    }

    static RefinementEvent fork(long task) {
        return new RefinementEvent(Kind.FORK, null, null, null, task);
    }

    Kind getKind() {
        return kind;
    }

    /** The subdivided element, or the receiver of the warning */
    GalerkinElement getReceiver() {
        return receiver;
    }

    GalerkinElement getSource() {
        return source;
    }

    Interaction getLink() {
        return link;
    }

    long getTask() {
        return task;
    }
}

/**
The events of the interactions refined by one worker, in the order in which
the worker refined them
*/
final class RefinementRecord {
    private final ArrayList<RefinementEvent> events = new ArrayList<>();
    private final ArrayList<GalerkinElement> nodesTooClose = new ArrayList<>();

    ArrayList<RefinementEvent> getEvents() {
        return events;
    }

    /** Pairs of the "nodes too close" warnings not yet turned into events */
    ArrayList<GalerkinElement> getNodesTooClose() {
        return nodesTooClose;
    }
}

/**
A task of a parallel Jacobi iteration: one interaction of an element list
(refined as refineInteraction does), or a sub-interaction between two
elements forked by a subdivision (refined as refineSubInteraction does). Once
run by a worker, the result: whether a top-level interaction was refined
(replaced by the links of its events) and the range [firstEvent, endEvent) of
its events in the record of that worker
*/
final class RefinementTask {
    private final Interaction interaction;
    private final GalerkinElement receiver;
    private final GalerkinElement source;
    private final ArrayList<RpkGeometry> candidatesList;
    private int worker;
    private long firstEvent;
    private long endEvent;
    private boolean refined;

    RefinementTask(Interaction inInteraction) {
        interaction = inInteraction;
        receiver = null;
        source = null;
        candidatesList = null;
        worker = -1;
        firstEvent = 0;
        endEvent = 0;
        refined = false;
    }

    RefinementTask(
        GalerkinElement inReceiver,
        GalerkinElement inSource,
        ArrayList<RpkGeometry> inCandidatesList)
    {
        interaction = null;
        receiver = inReceiver;
        source = inSource;
        candidatesList = inCandidatesList;
        worker = -1;
        firstEvent = 0;
        endEvent = 0;
        refined = false;
    }

    /** null for a sub-interaction */
    Interaction getInteraction() {
        return interaction;
    }

    GalerkinElement getReceiver() {
        return receiver;
    }

    GalerkinElement getSource() {
        return source;
    }

    ArrayList<RpkGeometry> getCandidatesList() {
        return candidatesList;
    }

    int getWorker() {
        return worker;
    }

    long getFirstEvent() {
        return firstEvent;
    }

    long getEndEvent() {
        return endEvent;
    }

    boolean isRefined() {
        return refined;
    }

    void setResult(int inWorker, long inFirstEvent, long inEndEvent, boolean inRefined) {
        worker = inWorker;
        firstEvent = inFirstEvent;
        endEvent = inEndEvent;
        refined = inRefined;
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
and none runs (a running task may fork more). Owns its tasks.
The results a worker sets in its tasks are read by the commit after the
workers are joined (Thread.join orders them before it).
*/
final class RefinementScheduler {
    private final ArrayList<RefinementTask> items;
    private final long numberOfItems;
    private final AtomicLong nextItem;
    private final long chunk;
    private final ReentrantLock lock;
    private final Condition condition;
    private final ArrayList<RefinementTask> forked;
    private final ArrayList<Integer> pending; // Indices of forked tasks
    private final AtomicLong pendingCount;
    private long running;
    private boolean failed;
    private final long forkThreshold;

    /**
    @param inItems the items; owned from now on
    @param inChunk number of items handed out at once
    @param inForkThreshold sub-interactions are forked while fewer tasks wait
    */
    RefinementScheduler(ArrayList<RefinementTask> inItems, long inChunk, long inForkThreshold) {
        items = inItems;
        numberOfItems = inItems.size();
        nextItem = new AtomicLong(0);
        chunk = inChunk;
        lock = new ReentrantLock();
        condition = lock.newCondition();
        forked = new ArrayList<>();
        pending = new ArrayList<>();
        pendingCount = new AtomicLong(0);
        running = 0;
        failed = false;
        forkThreshold = inForkThreshold;
    }

    /**
    Hands out the next chunk of items, [range[0], range[1])
    @return false when all items were handed out
    */
    boolean takeItems(long[] range) {
        if ( nextItem.get() >= numberOfItems ) {
            return false;
        }
        final long start = nextItem.getAndAdd(chunk);
        if ( start >= numberOfItems ) {
            return false;
        }
        range[0] = start;
        range[1] = start + chunk < numberOfItems ? start + chunk : numberOfItems;
        return true;
    }

    RefinementTask getItem(long index) {
        return items.get((int)index);
    }

    boolean wantsMoreTasks() {
        return nextItem.get() >= numberOfItems && pendingCount.get() < forkThreshold;
    }

    /**
    @return the index of the new forked task
    */
    long fork(RefinementTask task) {
        lock.lock();
        try {
            final long index = forked.size();
            forked.add(task);
            pending.add((int)index);
            pendingCount.incrementAndGet();
            condition.signal();
            return index;
        }
        finally {
            lock.unlock();
        }
    }

    /**
    Takes a forked task, waiting for one while other tasks run (call it once
    all items were handed out, and done() after running the task)
    @return the task, or null when all are done (or a worker failed)
    */
    RefinementTask takeForked() {
        lock.lock();
        try {
            while ( pending.isEmpty() && running > 0 && !failed ) {
                condition.awaitUninterruptibly();
            }
            RefinementTask task = null;
            if ( !pending.isEmpty() && !failed ) {
                task = forked.get(pending.get(pending.size() - 1));
                pending.remove(pending.size() - 1);
                pendingCount.decrementAndGet();
                running++;
            }
            else {
                condition.signalAll();
            }
            return task;
        }
        finally {
            lock.unlock();
        }
    }

    /** Called by a worker before running a chunk of items, which may fork */
    void startItems() {
        lock.lock();
        try {
            running++;
        }
        finally {
            lock.unlock();
        }
    }

    /** The forked task or chunk of items of the calling worker is done */
    void done() {
        lock.lock();
        try {
            running--;
            if ( running == 0 && pending.isEmpty() ) {
                condition.signalAll();
            }
        }
        finally {
            lock.unlock();
        }
    }

    /** The calling worker failed: the others stop */
    void fail() {
        lock.lock();
        try {
            running--;
            failed = true;
            nextItem.set(numberOfItems);
            condition.signalAll();
        }
        finally {
            lock.unlock();
        }
    }

    boolean hasFailed() {
        lock.lock();
        try {
            return failed;
        }
        finally {
            lock.unlock();
        }
    }

    /** After the workers ended */
    RefinementTask getForked(long index) {
        return forked.get((int)index);
    }
}

/**
An element whose interactions are refined in a parallel Jacobi iteration: its
tasks are numberOfItems consecutive ones from firstItem, in the order of its
interaction list when the iteration starts
*/
final class RefinementGroup {
    private final GalerkinElement element;
    private final long firstItem;
    private final long numberOfItems;

    RefinementGroup(GalerkinElement inElement, long inFirstItem, long inNumberOfItems) {
        element = inElement;
        firstItem = inFirstItem;
        numberOfItems = inNumberOfItems;
    }

    GalerkinElement getElement() {
        return element;
    }

    long getFirstItem() {
        return firstItem;
    }

    long getNumberOfItems() {
        return numberOfItems;
    }
}

/**
Mutable scratch of one thread of hierarchical refinement. The serial solver
uses the scene's query workspace, the solver's form factor workspace and no
record: light transport and interaction storage are done at once. A parallel
Jacobi worker has its own query, form factor and coefficient scratch, a record
receiving what is to be done later in the serial order, the lock shared
by the workers for regular subdivision and the scheduler of their tasks.
Nothing is owned.
*/
public final class RefinementWorkspace {
    private final SceneQueryWorkspace sceneQueryWorkspace;
    private final FormFactorWorkspace formFactorWorkspace;
    private final MemoryPool<double[]> coefficientsPool;
    private final RefinementRecord record;
    private final ReentrantLock subdivisionLock;
    private final RefinementScheduler scheduler;

    RefinementWorkspace(
        SceneQueryWorkspace inSceneQueryWorkspace,
        FormFactorWorkspace inFormFactorWorkspace,
        MemoryPool<double[]> inCoefficientsPool,
        RefinementRecord inRecord,
        ReentrantLock inSubdivisionLock,
        RefinementScheduler inScheduler)
    {
        sceneQueryWorkspace = inSceneQueryWorkspace;
        formFactorWorkspace = inFormFactorWorkspace;
        coefficientsPool = inCoefficientsPool;
        record = inRecord;
        subdivisionLock = inSubdivisionLock;
        scheduler = inScheduler;
    }

    SceneQueryWorkspace getSceneQueryWorkspace() {
        return sceneQueryWorkspace;
    }

    FormFactorWorkspace getFormFactorWorkspace() {
        return formFactorWorkspace;
    }

    MemoryPool<double[]> getCoefficientsPool() {
        return coefficientsPool;
    }

    /** null for the serial solver */
    RefinementRecord getRecord() {
        return record;
    }

    /** null for the serial solver */
    ReentrantLock getSubdivisionLock() {
        return subdivisionLock;
    }

    /** null for the serial solver */
    RefinementScheduler getScheduler() {
        return scheduler;
    }
}
