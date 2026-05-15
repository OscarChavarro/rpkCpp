const childProcess = require("node:child_process");

export class ProcessBuilder {
  private readonly command: string;

  public constructor(commandLine: string) {
    this.command = commandLine;
  }

  public startRead(): any {
    return ProcessBuilder.start(this.command, "r");
  }

  public startWrite(): any {
    return ProcessBuilder.start(this.command, "w");
  }

  public static start(commandLine: string, mode: string): any {
    if (!commandLine || commandLine.length === 0 || !mode || mode.length === 0) {
      return null;
    }

    const stdio = mode === "w"
      ? ["pipe", "inherit", "inherit"]
      : ["inherit", "pipe", "inherit"];

    return childProcess.spawn(commandLine, {
      shell: true,
      stdio
    });
  }

  public static close(processHandle: any): number {
    if (!processHandle) {
      return -1;
    }

    try {
      if (processHandle.stdin && typeof processHandle.stdin.end === "function") {
        processHandle.stdin.end();
      }
      if (typeof processHandle.kill === "function") {
        processHandle.kill();
      }
      return 0;
    }
    catch (_error) {
      return -1;
    }
  }
}
