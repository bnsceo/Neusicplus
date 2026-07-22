#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-ios"

cmake -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

open "${BUILD_DIR}/Neusicplus.xcodeproj"

echo "Opened the iOS Xcode project. In Signing & Capabilities, choose your Personal Team, connect your iPhone, select it as the run destination, then press Run."
