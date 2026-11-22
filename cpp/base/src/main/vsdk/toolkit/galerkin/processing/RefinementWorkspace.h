#ifndef REFINEMENT_WORKSPACE__
#define REFINEMENT_WORKSPACE__

#include "java/util/ArrayList.h"
#include "java/util/concurrent/atomic/AtomicLong.h"
#include "java/util/concurrent/locks/Condition.h"
#include "java/util/concurrent/locks/ReentrantLock.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"

class GalerkinElement;
class Interaction;
class RpkGeometry;

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
class RefinementEvent {
  public:
    enum Kind {
        SUBDIVISION,
        NODES_TOO_CLOSE,
        LINK,
        FORK
    };

    RefinementEvent() : kind(LINK), receiver(nullptr), source(nullptr), link(nullptr), task(0) {}

    static RefinementEvent
    subdivision(GalerkinElement *element) {
        return RefinementEvent(SUBDIVISION, element, nullptr, nullptr, 0);
    }

    static RefinementEvent
    nodesTooClose(const GalerkinElement *receiver, const GalerkinElement *source) {
        return RefinementEvent(NODES_TOO_CLOSE, receiver, source, nullptr, 0);
    }

    static RefinementEvent
    newLink(Interaction *link) {
        return RefinementEvent(LINK, nullptr, nullptr, link, 0);
    }

    static RefinementEvent
    fork(long int task) {
        return RefinementEvent(FORK, nullptr, nullptr, nullptr, task);
    }

    Kind getKind() const { return kind; }

    /** The subdivided element, or the receiver of the warning */
    const GalerkinElement *getReceiver() const { return receiver; }
    const GalerkinElement *getSource() const { return source; }
    Interaction *getLink() const { return link; }
    long int getTask() const { return task; }

  private:
    RefinementEvent(
        Kind inKind,
        const GalerkinElement *inReceiver,
        const GalerkinElement *inSource,
        Interaction *inLink,
        long int inTask):
        kind(inKind), receiver(inReceiver), source(inSource), link(inLink), task(inTask) {}

    Kind kind;
    const GalerkinElement *receiver;
    const GalerkinElement *source;
    Interaction *link;
    long int task;
};

/**
The events of the interactions refined by one worker, in the order in which
the worker refined them
*/
class RefinementRecord {
  public:
    RefinementRecord() : events(), nodesTooClose() {}
    RefinementRecord(const RefinementRecord &) = delete;
    RefinementRecord &operator=(const RefinementRecord &) = delete;

    java::ArrayList<RefinementEvent> *getEvents() { return &events; }

    /** Pairs of the "nodes too close" warnings not yet turned into events */
    java::ArrayList<const GalerkinElement *> *getNodesTooClose() { return &nodesTooClose; }

  private:
    java::ArrayList<RefinementEvent> events;
    java::ArrayList<const GalerkinElement *> nodesTooClose;
};

/**
A task of a parallel Jacobi iteration: one interaction of an element list
(refined as refineInteraction does), or a sub-interaction between two
elements forked by a subdivision (refined as refineSubInteraction does). Once
run by a worker, the result: whether a top-level interaction was refined
(replaced by the links of its events) and the range [firstEvent, endEvent) of
its events in the record of that worker
*/
class RefinementTask {
  public:
    explicit RefinementTask(Interaction *inInteraction):
        interaction(inInteraction), receiver(nullptr), source(nullptr), candidatesList(nullptr),
        worker(-1), firstEvent(0), endEvent(0), refined(false) {}

    RefinementTask(
        GalerkinElement *inReceiver,
        GalerkinElement *inSource,
        java::ArrayList<RpkGeometry *> *inCandidatesList):
        interaction(nullptr), receiver(inReceiver), source(inSource), candidatesList(inCandidatesList),
        worker(-1), firstEvent(0), endEvent(0), refined(false) {}

    RefinementTask(const RefinementTask &) = delete;
    RefinementTask &operator=(const RefinementTask &) = delete;

    /** nullptr for a sub-interaction */
    Interaction *getInteraction() const { return interaction; }
    GalerkinElement *getReceiver() const { return receiver; }
    GalerkinElement *getSource() const { return source; }
    java::ArrayList<RpkGeometry *> *getCandidatesList() const { return candidatesList; }
    int getWorker() const { return worker; }
    long int getFirstEvent() const { return firstEvent; }
    long int getEndEvent() const { return endEvent; }
    bool isRefined() const { return refined; }

    void
    setResult(int inWorker, long int inFirstEvent, long int inEndEvent, bool inRefined) {
        worker = inWorker;
        firstEvent = inFirstEvent;
        endEvent = inEndEvent;
        refined = inRefined;
    }

  private:
    Interaction *interaction;
    GalerkinElement *receiver;
    GalerkinElement *source;
    java::ArrayList<RpkGeometry *> *candidatesList;
    int worker;
    long int firstEvent;
    long int endEvent;
    bool refined;
};

/**
The tasks of a parallel Jacobi iteration. The items (interactions of the
element lists, fixed before the workers start) are handed out in chunks
through an atomic counter, without locking; the sub-interactions forked by
subdivisions go to a locked stack. Subdivisions fork only once the items are
all handed out, while few forked tasks wait, so that the many cheap items of
the late iterations cost no lock, and the few expensive ones of the early
iterations are shared. Workers end when no item is left, no forked task waits
and none runs (a running task may fork more). Owns its tasks.
*/
class RefinementScheduler {
  public:
    /**
    @param inItems the items; owned from now on
    @param inChunk number of items handed out at once
    @param inForkThreshold sub-interactions are forked while fewer tasks wait
    */
    RefinementScheduler(java::ArrayList<RefinementTask *> *inItems, long int inChunk, long int inForkThreshold):
        items(inItems), numberOfItems(inItems->size()), nextItem(0), chunk(inChunk),
        lock(), condition(lock), forked(), pending(), pendingCount(0), running(0), failed(false),
        forkThreshold(inForkThreshold) {}

    RefinementScheduler(const RefinementScheduler &) = delete;
    RefinementScheduler &operator=(const RefinementScheduler &) = delete;

    ~RefinementScheduler() {
        for ( long int i = 0; i < items->size(); i++ ) {
            delete items->get(i);
        }
        delete items;
        for ( long int i = 0; i < forked.size(); i++ ) {
            delete forked.get(i);
        }
    }

    /**
    Hands out the next chunk of items, [*first, *end)
    @return false when all items were handed out
    */
    bool
    takeItems(long int *first, long int *end) {
        if ( nextItem.get() >= numberOfItems ) {
            return false;
        }
        const long int start = nextItem.getAndAdd(chunk);
        if ( start >= numberOfItems ) {
            return false;
        }
        *first = start;
        *end = start + chunk < numberOfItems ? start + chunk : numberOfItems;
        return true;
    }

    RefinementTask *getItem(long int index) const { return items->get(index); }

    bool
    wantsMoreTasks() const {
        return nextItem.get() >= numberOfItems && pendingCount.get() < forkThreshold;
    }

    /**
    @return the index of the new forked task
    */
    long int
    fork(RefinementTask *task) {
        lock.lock();
        const long int index = forked.size();
        forked.add(task);
        pending.add(static_cast<int>(index));
        pendingCount.incrementAndGet();
        condition.signal();
        lock.unlock();
        return index;
    }

    /**
    Takes a forked task, waiting for one while other tasks run (call it once
    all items were handed out, and done() after running the task)
    @return the task, or nullptr when all are done (or a worker failed)
    */
    RefinementTask *
    takeForked() {
        lock.lock();
        while ( pending.isEmpty() && running > 0 && !failed ) {
            condition.await();
        }
        RefinementTask *task = nullptr;
        if ( !pending.isEmpty() && !failed ) {
            task = forked.get(pending.get(pending.size() - 1));
            pending.remove(pending.size() - 1);
            pendingCount.decrementAndGet();
            running++;
        } else {
            condition.signalAll();
        }
        lock.unlock();
        return task;
    }

    /** Called by a worker before running a chunk of items, which may fork */
    void
    startItems() {
        lock.lock();
        running++;
        lock.unlock();
    }

    /** The forked task or chunk of items of the calling worker is done */
    void
    done() {
        lock.lock();
        running--;
        if ( running == 0 && pending.isEmpty() ) {
            condition.signalAll();
        }
        lock.unlock();
    }

    /** The calling worker failed: the others stop */
    void
    fail() {
        lock.lock();
        running--;
        failed = true;
        nextItem.set(numberOfItems);
        condition.signalAll();
        lock.unlock();
    }

    bool hasFailed() const { return failed; }

    /** After the workers ended */
    const RefinementTask *getForked(long int index) const { return forked.get(index); }

  private:
    java::ArrayList<RefinementTask *> *items;
    long int numberOfItems;
    java::AtomicLong nextItem;
    long int chunk;
    java::ReentrantLock lock;
    java::Condition condition;
    java::ArrayList<RefinementTask *> forked;
    java::ArrayList<int> pending; // Indices of forked tasks (int: ArrayList<long>::remove would be ambiguous)
    java::AtomicLong pendingCount;
    long int running;
    bool failed;
    long int forkThreshold;
};

/**
An element whose interactions are refined in a parallel Jacobi iteration: its
tasks are numberOfItems consecutive ones from firstItem, in the order of its
interaction list when the iteration starts
*/
class RefinementGroup {
  public:
    RefinementGroup() : element(nullptr), firstItem(0), numberOfItems(0) {}
    RefinementGroup(const GalerkinElement *inElement, long int inFirstItem, long int inNumberOfItems):
        element(inElement), firstItem(inFirstItem), numberOfItems(inNumberOfItems) {}

    const GalerkinElement *getElement() const { return element; }
    long int getFirstItem() const { return firstItem; }
    long int getNumberOfItems() const { return numberOfItems; }

  private:
    const GalerkinElement *element;
    long int firstItem;
    long int numberOfItems;
};

/**
Mutable scratch of one thread of hierarchical refinement. The serial solver
uses the scene's query workspace, the solver's form factor workspace and no
record: light transport and interaction storage are done at once. A parallel
Jacobi worker has its own query, form factor and coefficient scratch, a record
receiving what is to be done later in the serial order, the lock shared
by the workers for regular subdivision and the scheduler of their tasks.
Nothing is owned.
*/
class RefinementWorkspace {
  public:
    RefinementWorkspace(
        SceneQueryWorkspace *inSceneQueryWorkspace,
        FormFactorWorkspace *inFormFactorWorkspace,
        MemoryPool<float> *inCoefficientsPool,
        RefinementRecord *inRecord,
        java::ReentrantLock *inSubdivisionLock,
        RefinementScheduler *inScheduler):
        sceneQueryWorkspace(inSceneQueryWorkspace),
        formFactorWorkspace(inFormFactorWorkspace),
        coefficientsPool(inCoefficientsPool),
        record(inRecord),
        subdivisionLock(inSubdivisionLock),
        scheduler(inScheduler) {}

    SceneQueryWorkspace *getSceneQueryWorkspace() const { return sceneQueryWorkspace; }
    FormFactorWorkspace *getFormFactorWorkspace() const { return formFactorWorkspace; }
    MemoryPool<float> *getCoefficientsPool() const { return coefficientsPool; }

    /** nullptr for the serial solver */
    RefinementRecord *getRecord() const { return record; }

    /** nullptr for the serial solver */
    java::ReentrantLock *getSubdivisionLock() const { return subdivisionLock; }

    /** nullptr for the serial solver */
    RefinementScheduler *getScheduler() const { return scheduler; }

  private:
    SceneQueryWorkspace *sceneQueryWorkspace;
    FormFactorWorkspace *formFactorWorkspace;
    MemoryPool<float> *coefficientsPool;
    RefinementRecord *record;
    java::ReentrantLock *subdivisionLock;
    RefinementScheduler *scheduler;
};

#endif
