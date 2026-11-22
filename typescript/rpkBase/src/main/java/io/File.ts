import { String as JavaString } from "../lang/String";

const fs = require("node:fs");
const pathModule = require("node:path");

export class File {
  private path: JavaString;

  public constructor(path?: string | JavaString) {
    if (path instanceof JavaString) {
      this.path = new JavaString(path);
    }
    else if (typeof path === "string") {
      this.path = new JavaString(path);
    }
    else {
      this.path = new JavaString();
    }
  }

  public dispose(): void {
    this.path.dispose();
  }

  public getName(): JavaString {
    return new JavaString(pathModule.basename(this.path.toCString()));
  }

  public exists(): boolean {
    const rawPath = this.path.toCString();
    if (!rawPath) {
      return false;
    }
    return fs.existsSync(rawPath);
  }

  public isDirectory(): boolean {
    const rawPath = this.path.toCString();
    if (!rawPath || !fs.existsSync(rawPath)) {
      return false;
    }
    try {
      const stats = fs.statSync(rawPath);
      return stats.isDirectory();
    }
    catch (_error) {
      return false;
    }
  }

  public isFile(): boolean {
    const rawPath = this.path.toCString();
    if (!rawPath || !fs.existsSync(rawPath)) {
      return false;
    }
    try {
      const stats = fs.statSync(rawPath);
      return stats.isFile();
    }
    catch (_error) {
      return false;
    }
  }

  public canRead(): boolean {
    const rawPath = this.path.toCString();
    if (!rawPath) {
      return false;
    }
    try {
      fs.accessSync(rawPath, fs.constants.R_OK);
      return true;
    }
    catch (_error) {
      return false;
    }
  }

  public canWrite(): boolean {
    const rawPath = this.path.toCString();
    if (!rawPath) {
      return false;
    }
    try {
      fs.accessSync(rawPath, fs.constants.W_OK);
      return true;
    }
    catch (_error) {
      return false;
    }
  }
}
