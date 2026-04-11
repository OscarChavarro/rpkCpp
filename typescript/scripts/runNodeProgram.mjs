#!/usr/bin/env node

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { resolve } from "node:path";

function printHelp() {
  console.log(`Usage:
  node scripts/runNodeProgram.mjs --entry <dist-entry.js> [options] [-- <app args...>]

Options:
  --entry <path>         Compiled entrypoint to execute (required)
  --build                Build project before execution
  --cwd <path>           Working directory for build/run (default: project root)
  --inspect              Run Node with --inspect
  --node-option <value>  Extra Node option (repeatable)
  --help                 Show this help
`);
}

const args = process.argv.slice(2);
let entry = "";
let shouldBuild = false;
let inspect = false;
let cwd = resolve(process.cwd());
const nodeOptions = [];
const appArgs = [];

let parsingAppArgs = false;
for (let i = 0; i < args.length; i++) {
  const arg = args[i];
  if (parsingAppArgs) {
    appArgs.push(arg);
    continue;
  }

  if (arg === "--") {
    parsingAppArgs = true;
    continue;
  }
  if (arg === "--help") {
    printHelp();
    process.exit(0);
  }
  if (arg === "--build") {
    shouldBuild = true;
    continue;
  }
  if (arg === "--inspect") {
    inspect = true;
    continue;
  }
  if (arg === "--entry") {
    entry = args[i + 1] ?? "";
    i++;
    continue;
  }
  if (arg === "--cwd") {
    cwd = resolve(args[i + 1] ?? cwd);
    i++;
    continue;
  }
  if (arg === "--node-option") {
    const value = args[i + 1] ?? "";
    if (value.length > 0) {
      nodeOptions.push(value);
    }
    i++;
    continue;
  }

  console.error(`Unknown option: ${arg}`);
  printHelp();
  process.exit(2);
}

if (!entry) {
  console.error("Missing required option: --entry");
  printHelp();
  process.exit(2);
}

if (shouldBuild) {
  const buildResult = spawnSync("npm", ["run", "build"], {
    cwd,
    stdio: "inherit"
  });
  if ((buildResult.status ?? 1) !== 0) {
    process.exit(buildResult.status ?? 1);
  }
}

const resolvedEntry = resolve(cwd, entry);
if (!existsSync(resolvedEntry)) {
  console.error(`Entrypoint does not exist: ${resolvedEntry}`);
  process.exit(2);
}

const nodeArgs = [];
if (inspect) {
  nodeArgs.push("--inspect");
}
nodeArgs.push(...nodeOptions, resolvedEntry, ...appArgs);

const result = spawnSync("node", nodeArgs, {
  cwd,
  stdio: "inherit"
});
process.exit(result.status ?? 1);
