declare const require: any;
declare const module: any;
declare const process: any;
declare const Buffer: any;

// The part of node:worker_threads used here (VITRAL's NodeWorkerExecutor and
// the parallel Jacobi replicas): rpk TS has no @types/node
declare module "node:worker_threads" {
  export const isMainThread: boolean;
  export const parentPort: {
    on(event: string, listener: (value: any) => void): void;
    postMessage(value: unknown): void;
  } | null;
  export class Worker {
    public constructor(filename: string | URL, options?: unknown);
    public on(event: string, listener: (value: any) => void): this;
    public off(event: string, listener: (value: any) => void): this;
    public postMessage(value: unknown): void;
    public terminate(): Promise<number>;
    public unref(): void;
  }
}
