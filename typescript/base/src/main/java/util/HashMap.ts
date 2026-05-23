import { HashMapEntry } from "./HashMapEntry";

export class HashMap<K, V> {
  private static readonly DEFAULT_CAPACITY = 16;
  private static readonly DEFAULT_LOAD_FACTOR = 0.75;
  private static nextObjectId = 1;
  private static readonly objectIds = new WeakMap<object, number>();

  private buckets: Array<HashMapEntry<K, V> | null>;
  private elementCount: number;
  private readonly maxLoadFactor: number;

  public constructor(initialCapacity = HashMap.DEFAULT_CAPACITY) {
    this.maxLoadFactor = HashMap.DEFAULT_LOAD_FACTOR;
    this.buckets = [];
    this.elementCount = 0;
    this.initialize(initialCapacity <= 0 ? HashMap.DEFAULT_CAPACITY : initialCapacity);
  }

  private initialize(initialBucketCount: number): void {
    this.buckets = new Array<HashMapEntry<K, V> | null>(initialBucketCount);
    this.buckets.fill(null);
    this.elementCount = 0;
  }

  public clear(): void {
    this.buckets.fill(null);
    this.elementCount = 0;
  }

  public size(): number {
    return this.elementCount;
  }

  public isEmpty(): boolean {
    return this.elementCount === 0;
  }

  private static hashFNV1a(text: string): number {
    let hash = 2166136261;
    for (let i = 0; i < text.length; i++) {
      hash ^= text.charCodeAt(i);
      hash = (hash * 16777619) >>> 0;
    }
    return hash >>> 0;
  }

  private static hashKeyValue(value: unknown): number {
    if (value === null || value === undefined) {
      return HashMap.hashFNV1a("null");
    }

    const valueType = typeof value;
    if (valueType === "string" || valueType === "number" || valueType === "boolean" || valueType === "bigint") {
      return HashMap.hashFNV1a(`${valueType}:${globalThis.String(value)}`);
    }

    if (valueType === "object" || valueType === "function") {
      const keyObject = value as object;
      let id = HashMap.objectIds.get(keyObject);
      if (id === undefined) {
        id = HashMap.nextObjectId++;
        HashMap.objectIds.set(keyObject, id);
      }
      return HashMap.hashFNV1a(`object:${id}`);
    }

    return HashMap.hashFNV1a(`${valueType}:unknown`);
  }

  private bucketIndexFor(key: K): number {
    if (this.buckets.length <= 0) {
      return 0;
    }
    return HashMap.hashKeyValue(key) % this.buckets.length;
  }

  public containsKey(key: K): boolean {
    return this.tryGet(key);
  }

  public tryGet(key: K, valueOut?: { value: V }): boolean {
    if (this.buckets.length <= 0) {
      return false;
    }

    const index = this.bucketIndexFor(key);
    let current = this.buckets[index] ?? null;
    while (current !== null) {
      if (current.key === key) {
        if (valueOut !== undefined) {
          valueOut.value = current.value;
        }
        return true;
      }
      current = current.next;
    }
    return false;
  }

  public getOrDefault(key: K, defaultValue: V): V {
    const out = { value: defaultValue };
    if (this.tryGet(key, out)) {
      return out.value;
    }
    return defaultValue;
  }

  private rehash(newBucketCount: number): void {
    if (newBucketCount <= this.buckets.length) {
      return;
    }

    const oldBuckets = this.buckets;
    this.buckets = new Array<HashMapEntry<K, V> | null>(newBucketCount);
    this.buckets.fill(null);

    for (const head of oldBuckets) {
      let current = head;
      while (current !== null) {
        const next = current.next;
        const index = this.bucketIndexFor(current.key);
        current.next = this.buckets[index] ?? null;
        this.buckets[index] = current;
        current = next;
      }
    }
  }

  public put(key: K, value: V): boolean {
    if (this.buckets.length <= 0) {
      this.initialize(HashMap.DEFAULT_CAPACITY);
    }

    const index = this.bucketIndexFor(key);
    let current = this.buckets[index] ?? null;
    while (current !== null) {
      if (current.key === key) {
        current.value = value;
        return true;
      }
      current = current.next;
    }

    const entry = new HashMapEntry(key, value, this.buckets[index] ?? null);
    this.buckets[index] = entry;
    this.elementCount++;

    if (this.elementCount > this.buckets.length * this.maxLoadFactor) {
      this.rehash(this.buckets.length * 2);
    }

    return true;
  }

  public remove(key: K): boolean {
    if (this.buckets.length <= 0) {
      return false;
    }

    const index = this.bucketIndexFor(key);
    let current = this.buckets[index] ?? null;
    let previous: HashMapEntry<K, V> | null = null;

    while (current !== null) {
      if (current.key === key) {
        if (previous === null) {
          this.buckets[index] = current.next;
        }
        else {
          previous.next = current.next;
        }
        this.elementCount--;
        return true;
      }
      previous = current;
      current = current.next;
    }

    return false;
  }
}
