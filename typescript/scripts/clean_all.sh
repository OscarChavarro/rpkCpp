#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

rm -rf output/*
rm -rf dist
rm -rf node_modules
rm -rf base/dist
rm -rf base/node_modules
rm -rf testsuite/ApplicationCases/RenderparkApplication/dist
rm -rf testsuite/ApplicationCases/RenderparkApplication/node_modules
rm -f package-lock.json
rm -f base/package-lock.json
rm -f testsuite/ApplicationCases/RenderparkApplication/package-lock.json
