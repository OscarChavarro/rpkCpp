/*
Imported first by the worker module of a parallel Jacobi replica (see
RefinementReplicaMain): a replica runs this program a second time, with the
same command line, only to hold a copy of its state, so it must write
nothing. Every file it opens for writing is /dev/null, its standard output and
error and console go nowhere, and the commands it would pipe output to are not
run. It must run before any other module is loaded (System opens /dev/stdout
when it loads).
*/
const sandboxWorkerThreads = require("node:worker_threads");

if (!sandboxWorkerThreads.isMainThread) {
  const fs = require("node:fs");
  const childProcess = require("node:child_process");

  const opensForWriting = (flags: unknown): boolean => {
    if (typeof flags === "number") {
      return (flags & (fs.constants.O_WRONLY | fs.constants.O_RDWR)) !== 0;
    }
    return typeof flags === "string" && /[wa+]/.test(flags);
  };
  const openSync = fs.openSync;
  fs.openSync = (path: unknown, flags?: unknown, mode?: unknown): number =>
    opensForWriting(flags) ? openSync("/dev/null", "w") : openSync(path, flags, mode);
  const writeSync = fs.writeSync;
  fs.writeSync = (fd: number, buffer: { length: number }, ...rest: unknown[]): number => {
    if (fd === 1 || fd === 2) {
      return typeof buffer === "string" ? buffer.length : (typeof rest[1] === "number" ? rest[1] : buffer.length);
    }
    return writeSync(fd, buffer, ...rest);
  };
  fs.writeFileSync = (): void => {};
  fs.appendFileSync = (): void => {};

  childProcess.spawnSync = (): unknown => ({ pid: 0, output: [], stdout: null, stderr: null, status: 0, signal: null });
  const spawn = childProcess.spawn;
  childProcess.spawn = (command: string, options?: { stdio?: unknown[] }): unknown => {
    if (options !== undefined && Array.isArray(options.stdio) && options.stdio[0] === "pipe") {
      return spawn("cat > /dev/null", { shell: true, stdio: ["pipe", "ignore", "ignore"] });
    }
    return spawn(command, options);
  };

  process.stdout.write = (): boolean => true;
  process.stderr.write = (): boolean => true;
  const silent = (): void => {};
  const replicaConsole = (globalThis as unknown as { console: Record<string, unknown> }).console;
  for (const method of ["log", "info", "warn", "error", "debug", "trace"]) {
    replicaConsole[method] = silent;
  }
}

export {};
