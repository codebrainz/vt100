#!/usr/bin/env bash
set -e

# Get the directory of this script (project root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$BUILD_DIR/install"

# Clean build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Configure build with tests and docs enabled, install prefix set to $INSTALL_DIR
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DBUILD_TESTS=ON \
    -DBUILD_DOCS=ON \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"

# Build all targets with parallel jobs, including docs
cmake --build "$BUILD_DIR" -j"$(nproc)" --target all doc

# Install to $INSTALL_DIR
cmake --install "$BUILD_DIR"

# Run tests
if [ -x "$BUILD_DIR/bin/vt100_test" ]; then
    echo "Running unit tests..."
    "$BUILD_DIR/bin/vt100_test"
else
    echo "Test binary not found!"
    exit 1
fi

# Show installed files
if command -v tree >/dev/null 2>&1; then
    echo "Installed files in $INSTALL_DIR:"
    cd "$INSTALL_DIR" && tree -lh .
fi
