import type { Entity } from "./Entity.js";

/** Kinds of events an entity can emit (Java `EntityEvent.Type`). */
export enum EntityEventType {
    /** Some attribute of the entity changed, see `Entity.update()` */
    UPDATED = 0,
    /** The entity was discarded, see `Entity.dispose()` */
    DELETED = 1,
}

/**
Event emitted by an `Entity` to its subscribers (see
`Entity.addEntityListener`) when the entity changes or is discarded.
*/
export class EntityEvent {
    private readonly source: Entity;
    private readonly type: EntityEventType;

    /**
    @param source entity that emits the event
    @param type kind of event
    */
    public constructor(source: Entity, type: EntityEventType) {
        this.source = source;
        this.type = type;
    }

    /** @return the entity that emitted the event */
    public getSource(): Entity {
        return this.source;
    }

    /** @return the kind of event */
    public getType(): EntityEventType {
        return this.type;
    }
}
