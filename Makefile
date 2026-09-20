CMAKE ?= cmake
BUILD_DIR ?= build
BUILD_TYPE ?= Release
PREFIX ?= /usr/local

.PHONY: all configure run clean native install-native deb

all: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBACKYARD_BUILD_PROFILE=generic

run: all
	./$(BUILD_DIR)/backyard-racer

native:
	$(CMAKE) -S . -B build-native -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBACKYARD_BUILD_PROFILE=native
	$(CMAKE) --build build-native --parallel
	ctest --test-dir build-native --output-on-failure

install-native: native
	$(CMAKE) --install build-native --component Runtime --prefix "$(PREFIX)"

deb:
	bash packaging/build-deb.sh

clean:
	rm -rf $(BUILD_DIR) build-native build-package dist
