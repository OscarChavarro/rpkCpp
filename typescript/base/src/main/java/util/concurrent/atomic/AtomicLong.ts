/** Atomic-long API for worker-isolated state. Values use bigint to retain Java long precision. */
export class AtomicLong {
    private value: bigint;
    public constructor(initialValue = 0n) {
        this.value = BigInt.asIntN(64, initialValue);
    }
    public get(): bigint {
        return this.value;
    }
    public set(value: bigint): void {
        this.value = BigInt.asIntN(64, value);
    }
    public lazySet(value: bigint): void {
        this.set(value);
    }
    public getAndSet(value: bigint): bigint {
        const prior = this.value;
        this.set(value);
        return prior;
    }
    public compareAndSet(expected: bigint, update: bigint): boolean {
        if (this.value !== BigInt.asIntN(64, expected)) return false;
        this.set(update);
        return true;
    }
    public getAndIncrement(): bigint {
        return this.getAndAdd(1n);
    }
    public getAndDecrement(): bigint {
        return this.getAndAdd(-1n);
    }
    public incrementAndGet(): bigint {
        return this.addAndGet(1n);
    }
    public decrementAndGet(): bigint {
        return this.addAndGet(-1n);
    }
    public getAndAdd(delta: bigint): bigint {
        const prior = this.value;
        this.value = BigInt.asIntN(64, this.value + delta);
        return prior;
    }
    public addAndGet(delta: bigint): bigint {
        this.value = BigInt.asIntN(64, this.value + delta);
        return this.value;
    }
    public toString(): string {
        return this.value.toString();
    }
}
