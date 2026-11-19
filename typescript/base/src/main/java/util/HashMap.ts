import { IllegalStateException } from "../lang/IllegalStateException.js";

export interface MapEntry<K, V> {
    readonly key: K;
    value: V;
}

interface HashMapSlot<K, V> extends MapEntry<K, V> {
    /**
    The key's `hashCode()` as it stood when the slot was created, mirroring
    the `hash` field a `java.util.HashMap.Node` caches at insertion time.
    `undefined` marks a key that exposes no `hashCode()`, which is matched by
    `equals` alone. Caching matters for keys that callers mutate after
    insertion: Java then keeps the slot reachable only under its original
    hash, so a lookup by an equal-but-differently-hashed key misses, and so
    must this map.
    */
    readonly hash: number | undefined;
}

/** Java Map semantics with Java-style equals dispatch for object keys. */
export class HashMap<K, V> implements Iterable<MapEntry<K, V>> {
    private readonly entriesArray: HashMapSlot<K, V>[] = [];
    /**
    Slots grouped by their cached hash, playing the role of a
    `java.util.HashMap` bucket table: a lookup only ever compares against the
    slots that hashed the same way. Keys with no `hashCode()` are absent here
    and fall back to a scan of `entriesArray`.
    */
    private readonly buckets = new Map<number, HashMapSlot<K, V>[]>();
    private modificationCount = 0;

    public constructor(initial?: Iterable<readonly [K, V]>) {
        if (initial !== undefined) for (const [key, value] of initial) this.put(key, value);
    }
    public size(): number {
        return this.entriesArray.length;
    }
    public isEmpty(): boolean {
        return this.entriesArray.length === 0;
    }
    public containsKey(key: K): boolean {
        return this.findSlot(key) !== undefined;
    }
    public containsValue(value: V): boolean {
        return this.entriesArray.some((entry) => HashMap.javaEquals(entry.value, value));
    }
    public get(key: K): V | undefined {
        return this.findSlot(key)?.value;
    }
    public getOrDefault(key: K, defaultValue: V): V {
        const slot = this.findSlot(key);
        return slot === undefined ? defaultValue : slot.value;
    }
    public tryGet(key: K, valueOut?: { value: V }): boolean {
        const slot = this.findSlot(key);
        if (slot === undefined) return false;
        if (valueOut !== undefined) valueOut.value = slot.value;
        return true;
    }

    /** Java Map.put: returns the prior value, or undefined for Java null/absent. */
    public put(key: K, value: V): V | undefined {
        const existing = this.findSlot(key);
        if (existing !== undefined) {
            const previous = existing.value;
            existing.value = value;
            return previous;
        }
        const hash = HashMap.javaHashCode(key);
        const slot: HashMapSlot<K, V> = { key, value, hash };
        this.entriesArray.push(slot);
        if (hash !== undefined) {
            const bucket = this.buckets.get(hash);
            if (bucket === undefined) this.buckets.set(hash, [slot]);
            else bucket.push(slot);
        }
        this.modificationCount++;
        return undefined;
    }

    public putAll(entries: Iterable<readonly [K, V]>): void {
        for (const [key, value] of entries) this.put(key, value);
    }
    /** Java Map.remove: returns the prior value, or undefined for Java null/absent. */
    public remove(key: K): V | undefined {
        const slot = this.findSlot(key);
        if (slot === undefined) return undefined;
        this.entriesArray.splice(this.entriesArray.indexOf(slot), 1);
        if (slot.hash !== undefined) {
            const bucket = this.buckets.get(slot.hash);
            if (bucket !== undefined) {
                bucket.splice(bucket.indexOf(slot), 1);
                if (bucket.length === 0) this.buckets.delete(slot.hash);
            }
        }
        this.modificationCount++;
        return slot.value;
    }
    public clear(): void {
        if (this.entriesArray.length > 0) {
            this.entriesArray.length = 0;
            this.buckets.clear();
            this.modificationCount++;
        }
    }
    public keySet(): K[] {
        return this.entriesArray.map((entry) => entry.key);
    }
    public values(): V[] {
        return this.entriesArray.map((entry) => entry.value);
    }
    public entrySet(): MapEntry<K, V>[] {
        return this.entriesArray.map((entry) => ({ key: entry.key, value: entry.value }));
    }

    public *[Symbol.iterator](): IterableIterator<MapEntry<K, V>> {
        const expectedModificationCount = this.modificationCount;
        for (const entry of this.entriesArray) {
            if (this.modificationCount !== expectedModificationCount)
                throw new IllegalStateException("Map modified during iteration");
            yield entry;
        }
    }

    private findSlot(key: K): HashMapSlot<K, V> | undefined {
        const hash = HashMap.javaHashCode(key);
        if (hash === undefined) {
            return this.entriesArray.find((entry) => entry.hash === undefined && HashMap.javaEquals(entry.key, key));
        }
        return this.buckets.get(hash)?.find((entry) => HashMap.javaEquals(entry.key, key));
    }
    private static javaHashCode(key: unknown): number | undefined {
        if (
            key !== null &&
            typeof key === "object" &&
            "hashCode" in key &&
            typeof (key as { hashCode?: unknown }).hashCode === "function"
        )
            return (key as { hashCode(): number }).hashCode();
        return undefined;
    }
    private static javaEquals(left: unknown, right: unknown): boolean {
        if (
            left === right ||
            (typeof left === "number" && typeof right === "number" && Number.isNaN(left) && Number.isNaN(right))
        )
            return true;
        if (
            left !== null &&
            typeof left === "object" &&
            "equals" in left &&
            typeof (left as { equals?: unknown }).equals === "function"
        )
            return (left as { equals(other: unknown): boolean }).equals(right);
        return false;
    }
}
