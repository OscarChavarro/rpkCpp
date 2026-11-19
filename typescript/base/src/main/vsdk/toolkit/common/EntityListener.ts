import type { EntityEvent } from "./EntityEvent.js";

/**
Subscriber of the events emitted by an `Entity`. Objects that depend on an
entity (editors, views, caches...) implement this interface and subscribe
with `Entity.addEntityListener` to learn when it changes or is discarded.
*/
export interface EntityListener {
    /**
    Called by the entity each time it emits an event.
    @param event the event, with its source entity and type
    */
    notifyEntityEvent(event: EntityEvent): void;
}
