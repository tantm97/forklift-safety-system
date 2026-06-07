# forklift-safety-system — developer Makefile (thin wrapper over CMake)

BUILD_DIR ?= build
BUILD_TYPE ?= Release
JOBS ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

.PHONY: configure build run test fmt tidy clean install-hooks setup-test run-test run-rtsp deploy-jetson

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

run: build
	./$(BUILD_DIR)/forklift_safety --config conf/system.yaml

# Laptop test with a local video file (downloads model + video on first run).
run-test:
	./scripts/run_test.sh

# Laptop/production run against a live RTSP camera: make run-rtsp RTSP=rtsp://...
run-rtsp:
	./scripts/run_rtsp.sh "$(RTSP)"

setup-test:
	./scripts/setup_test.sh

# Build + install as a systemd service on a Jetson Nano / Debian box.
deploy-jetson:
	sudo ./deploy/install_jetson.sh

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

fmt:
	@find include src tests -name '*.h' -o -name '*.cpp' | xargs clang-format -i

tidy:
	@run-clang-tidy -p $(BUILD_DIR) include src

clean:
	rm -rf $(BUILD_DIR)
