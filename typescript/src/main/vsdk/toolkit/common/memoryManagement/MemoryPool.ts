export namespace MemoryPool {
  type PoolBlock<T> = {
    buffer: T[];
    capacityElements: number;
    usedElements: number;
    prev: PoolBlock<T> | null;
    next: PoolBlock<T> | null;
  };

  export class Generic<T> {
    private readonly supplier: () => T;
    private head: PoolBlock<T> | null;
    private tail: PoolBlock<T> | null;
    private current: PoolBlock<T> | null;
    private initialized: boolean;

    public constructor(supplier: () => T) {
      this.supplier = supplier;
      this.head = null;
      this.tail = null;
      this.current = null;
      this.initialized = false;
    }

    private createBlock(capacityElements: number): PoolBlock<T> | null {
      if (capacityElements <= 0) {
        return null;
      }
      const buffer = new Array<T>(capacityElements);
      for (let i = 0; i < capacityElements; i++) {
        buffer[i] = this.supplier();
      }
      return {
        buffer,
        capacityElements,
        usedElements: 0,
        prev: null,
        next: null,
      };
    }

    public init(sizeInBytes: number): void {
      if (this.initialized || sizeInBytes <= 0) {
        return;
      }
      let elements = globalThis.Math.floor(sizeInBytes);
      if (elements <= 0) {
        elements = 1;
      }
      const first = this.createBlock(elements);
      if (first === null) {
        return;
      }
      this.head = first;
      this.tail = first;
      this.current = first;
      this.initialized = true;
    }

    public allocate(numberOfElements: number): T | null {
      if (!this.initialized || numberOfElements <= 0) {
        return null;
      }
      const request = globalThis.Math.floor(numberOfElements);
      if (request <= 0) {
        return null;
      }

      let scan = this.current !== null ? this.current : this.tail;
      while (scan !== null) {
        if (scan.usedElements + request <= scan.capacityElements) {
          const out = scan.buffer[scan.usedElements];
          scan.usedElements += request;
          this.current = scan;
          return out;
        }
        scan = scan.next;
      }
      return null;
    }

    public free(numberOfElements: number): void {
      if (!this.initialized || numberOfElements <= 0) {
        return;
      }
      const release = globalThis.Math.floor(numberOfElements);
      let remaining = release;
      let scan = this.current !== null ? this.current : this.tail;
      while (scan !== null && remaining > 0) {
        if (scan.usedElements >= remaining) {
          scan.usedElements -= remaining;
          remaining = 0;
          this.current = scan;
        }
        else {
          remaining -= scan.usedElements;
          scan.usedElements = 0;
          scan = scan.prev;
          this.current = scan;
        }
      }
      if (this.current === null) {
        this.current = this.head;
      }
    }

    public clear(): void {
      for (let it = this.head; it !== null; it = it.next) {
        it.usedElements = 0;
      }
      this.current = this.head;
    }

    public owns(value: T): boolean {
      for (let block = this.head; block !== null; block = block.next) {
        for (let i = 0; i < block.capacityElements; i++) {
          if (block.buffer[i] === value) {
            return true;
          }
        }
      }
      return false;
    }

    public expand(numberOfElements: number): boolean {
      if (numberOfElements <= 0) {
        return true;
      }
      const grow = globalThis.Math.floor(numberOfElements);
      if (grow <= 0) {
        return false;
      }
      const newBlock = this.createBlock(grow);
      if (newBlock === null) {
        return false;
      }
      if (this.head === null) {
        this.head = newBlock;
        this.tail = newBlock;
        this.current = newBlock;
      }
      else {
        newBlock.prev = this.tail;
        if (this.tail !== null) {
          this.tail.next = newBlock;
        }
        this.tail = newBlock;
        this.current = newBlock;
      }
      this.initialized = true;
      return true;
    }
  }
}
