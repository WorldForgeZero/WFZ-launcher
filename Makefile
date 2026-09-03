MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)

CXX := g++

SRC_DIR := src
BUILD_DIR := build
DIST_DIR := dist

TARGET := wfz_launcher

RAYLIB_ROOT := third_party/raylib/linux_amd64
MBEDTLS_ROOT := third_party/mbedtls/linux_amd64

CPP_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.cpp' -print)

DEBUG_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/debug/%.o,$(CPP_SOURCES))
RELEASE_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/release/%.o,$(CPP_SOURCES))

INCLUDES := \
	-I$(RAYLIB_ROOT)/include \
	-I$(MBEDTLS_ROOT)/include \
	-Ithird_party

COMMON_CXXFLAGS := \
	-std=c++17 \
	-Wall \
	-Wextra \
	$(INCLUDES)

DEBUG_CXXFLAGS := \
	$(COMMON_CXXFLAGS) \
	-g \
	-O0

RELEASE_CXXFLAGS := \
	$(COMMON_CXXFLAGS) \
	-g \
	-O3 \
	-flto \
	-fno-omit-frame-pointer \
	-DNDEBUG

BASE_LDFLAGS := \
	$(RAYLIB_ROOT)/lib/libraylib.a \
	$(MBEDTLS_ROOT)/lib/libmbedtls.a \
	$(MBEDTLS_ROOT)/lib/libmbedx509.a \
	$(MBEDTLS_ROOT)/lib/libmbedcrypto.a \
	$(MBEDTLS_ROOT)/lib/libtfpsacrypto.a \
	-lGL \
	-lm \
	-lpthread \
	-ldl \
	-lrt \
	-lX11

DEBUG_LDFLAGS := $(BASE_LDFLAGS)
RELEASE_LDFLAGS := $(BASE_LDFLAGS) -flto

.PHONY: all debug release run run-release clean clear dist dist-release

all: debug

clear: clean

$(BUILD_DIR)/debug/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(DEBUG_CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/release/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(RELEASE_CXXFLAGS) -MMD -MP -c $< -o $@

debug: $(DEBUG_OBJS)
	$(CXX) $(DEBUG_OBJS) $(DEBUG_LDFLAGS) -o $(TARGET)

release: $(RELEASE_OBJS)
	$(CXX) $(RELEASE_OBJS) $(RELEASE_LDFLAGS) -o $(TARGET)

run: debug
	./$(TARGET)

run-release: release
	./$(TARGET)

DIST_MODE ?= debug

dist: $(DIST_MODE)
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)
	cp $(TARGET) $(DIST_DIR)/
	@echo "Distribution built in $(DIST_DIR)/ (mode: $(DIST_MODE))"

dist-release:
	$(MAKE) dist DIST_MODE=release

-include $(DEBUG_OBJS:.o=.d)
-include $(RELEASE_OBJS:.o=.d)

clean:
	rm -f $(TARGET)
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)
