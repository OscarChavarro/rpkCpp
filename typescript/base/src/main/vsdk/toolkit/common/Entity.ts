import { EntityEvent, EntityEventType } from "./EntityEvent.js";
import type { EntityListener } from "./EntityListener.js";
import type { ModelElement } from "./ModelElement.js";

/** Base class for serializable elements of Vitral's model. */
export class Entity implements ModelElement {
    public static readonly BYTE_SIZE_IN_BYTES = 1;
    public static readonly INT_SIZE_IN_BYTES = 4;
    public static readonly LONG_SIZE_IN_BYTES = 8;
    public static readonly FLOAT_SIZE_IN_BYTES = 4;
    public static readonly DOUBLE_SIZE_IN_BYTES = 8;
    public static readonly VECTOR3D_SIZE_IN_BYTES = 24;
    public static readonly COLORRGB_SIZE_IN_BYTES = 24;
    public static readonly POINTER_SIZE_IN_BYTES = 8;

    /**
    Control specifications used by reflection-based generic editors to build
    GUI dialogs for this entity. Kept null until first used, so fine-grained
    entities (vertices, half-edges, ...) do not pay for an empty list.
    */
    private controlSpecifications: string[] | null = null;

    /**
    Subscribers notified by `update()` and `dispose()`; kept null until the
    first subscription. Not part of the model (Java marks it transient).
    */
    private entityListeners: EntityListener[] | null = null;

    public getSizeInBytes(): number {
        return 0;
    }

    /**
    Returns the control specifications used by reflection-based generic
    editors, creating an empty list on first access.
    @return the mutable list of control specifications for this entity
    */
    public getControlSpecifications(): string[] {
        if (this.controlSpecifications === null) {
            this.controlSpecifications = [];
        }
        return this.controlSpecifications;
    }

    /**
    Replaces the control specifications used by reflection-based generic
    editors.
    @param controlSpecifications new list of control specifications; may be
    null to release the current list
    */
    public setControlSpecifications(controlSpecifications: string[] | null): void {
        this.controlSpecifications = controlSpecifications;
    }

    /**
    Subscribes a listener to the events of this entity. Adding the same
    listener twice has no effect.
    @param listener object to notify; null is ignored
    */
    public addEntityListener(listener: EntityListener | null): void {
        if (listener === null) {
            return;
        }
        if (this.entityListeners === null) {
            this.entityListeners = [];
        }
        if (!this.entityListeners.includes(listener)) {
            this.entityListeners.push(listener);
        }
    }

    /**
    Unsubscribes a listener from the events of this entity.
    @param listener object to stop notifying
    */
    public removeEntityListener(listener: EntityListener): void {
        if (this.entityListeners === null) {
            return;
        }
        const index = this.entityListeners.indexOf(listener);
        if (index >= 0) {
            this.entityListeners.splice(index, 1);
        }
    }

    /**
    Notifies the subscribers that this entity changed. Code that modifies an
    entity (editors, tools) calls this method after the change, so the
    objects that depend on the entity can refresh.
    */
    public update(): void {
        this.fireEntityEvent(EntityEventType.UPDATED);
    }

    /**
    Notifies the subscribers that this entity was discarded (for example,
    deleted from a scene) and then drops all of them. This is the counterpart
    of emitting the event from a C++ destructor: the entity data is kept, so
    an undo operation can still insert it again.
    */
    public dispose(): void {
        this.fireEntityEvent(EntityEventType.DELETED);
        this.entityListeners = null;
    }

    private fireEntityEvent(type: EntityEventType): void {
        if (this.entityListeners === null || this.entityListeners.length === 0) {
            return;
        }
        const event = new EntityEvent(this, type);
        // Copy: listeners may unsubscribe while being notified
        const listeners = [...this.entityListeners];
        for (const listener of listeners) {
            listener.notifyEntityEvent(event);
        }
    }

    /** Pairs public getX/setX methods, as Java reflection does for this API. */
    public getEncapsulatedVariables(): string[] {
        const getters = new Map<string, () => unknown>();
        const setters = new Set<string>();
        let prototype: object | null = Object.getPrototypeOf(this);
        while (prototype !== null && prototype !== Entity.prototype) {
            for (const name of Object.getOwnPropertyNames(prototype)) {
                const descriptor = Object.getOwnPropertyDescriptor(prototype, name);
                if (typeof descriptor?.value !== "function") continue;
                if (name.startsWith("get") && name.length > 3 && descriptor.value.length === 0)
                    getters.set(name.slice(3), descriptor.value.bind(this) as () => unknown);
                if (name.startsWith("set") && name.length > 3 && descriptor.value.length === 1)
                    setters.add(name.slice(3));
            }
            prototype = Object.getPrototypeOf(prototype);
        }
        const variables: string[] = [];
        for (const [name, getter] of getters) {
            if (!setters.has(name)) continue;
            const type = Entity.supportedTypeName(getter());
            if (type !== undefined) variables.push(`${type}:${name.charAt(0).toLowerCase()}${name.slice(1)}`);
        }
        return variables;
    }

    private static supportedTypeName(value: unknown): string | undefined {
        switch (typeof value) {
            case "number":
                return "double";
            case "boolean":
                return "boolean";
            case "string":
                return "java.lang.String";
            case "object":
                return value === null ? undefined : (value as { constructor: { name: string } }).constructor.name;
            default:
                return undefined;
        }
    }

    /** Mirrors the current Java implementation, whose clone contract returns null. */
    public clone(): unknown {
        return null;
    }
}
