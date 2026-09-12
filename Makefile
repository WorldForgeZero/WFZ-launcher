MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)

# Compilers
CXX := g++
WIN_CXX := x86_64-w64-mingw32-g++

LD := ld
WIN_LD := x86_64-w64-mingw32-ld

# Directories
SRC_DIR := src
BUILD_DIR := build
DIST_DIR := dist

# Targets
TARGET := wfz_launcher
WIN_TARGET := wfz_launcher.exe

# Assets
FONT_FILE := assets/fonts/Monocraft.ttf

# Third-party dependencies
RAYLIB_ROOT := third_party/raylib/linux_amd64
OPENSSL_ROOT := third_party/openssl/linux_amd64

WIN_RAYLIB_ROOT := third_party/raylib/win64_mingw
WIN_OPENSSL_ROOT := third_party/openssl/win64_mingw

# Sources
CPP_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.cpp' -print)

# Objects
DEBUG_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/debug/%.o,$(CPP_SOURCES))

RELEASE_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/release/%.o,$(CPP_SOURCES))

WIN_DEBUG_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/win-debug/%.o,$(CPP_SOURCES))

WIN_RELEASE_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/win-release/%.o,$(CPP_SOURCES))

# Embedded assets
RELEASE_FONT_OBJ := \
	$(BUILD_DIR)/release/embedded/monocraft_font.o

WIN_RELEASE_FONT_OBJ := \
	$(BUILD_DIR)/win-release/embedded/monocraft_font.o

# Includes
INCLUDES := \
	-I$(SRC_DIR) \
	-I$(RAYLIB_ROOT)/include \
	-I$(OPENSSL_ROOT)/include \
	-Ithird_party

WIN_INCLUDES := \
	-I$(SRC_DIR) \
	-I$(WIN_RAYLIB_ROOT)/include \
	-I$(WIN_OPENSSL_ROOT)/include \
	-Ithird_party

# Common compiler flags
COMMON_CXXFLAGS := \
	-std=c++17 \
	-Wall \
	-Wextra \
	-pipe \
	-DCPPHTTPLIB_OPENSSL_SUPPORT \
	$(INCLUDES)

WIN_COMMON_CXXFLAGS := \
	-std=c++17 \
	-Wall \
	-Wextra \
	-pipe \
	-DCPPHTTPLIB_OPENSSL_SUPPORT \
	$(WIN_INCLUDES)

# Debug compiler flags
DEBUG_CXXFLAGS := \
	$(COMMON_CXXFLAGS) \
	-g \
	-O0

WIN_DEBUG_CXXFLAGS := \
	$(WIN_COMMON_CXXFLAGS) \
	-g \
	-O0

# Release compiler flags
RELEASE_CXXFLAGS := \
	$(COMMON_CXXFLAGS) \
	-DEMBEDED_FONT \
	-DNDEBUG \
	-O3 \
	-flto=auto \
	-ffunction-sections \
	-fdata-sections

WIN_RELEASE_CXXFLAGS := \
	$(WIN_COMMON_CXXFLAGS) \
	-DEMBEDED_FONT \
	-DNDEBUG \
	-O3 \
	-ffunction-sections \
	-fdata-sections

# Linux linker flags
BASE_LDFLAGS := \
	$(RAYLIB_ROOT)/lib/libraylib.a \
	$(OPENSSL_ROOT)/lib/libssl.a \
	$(OPENSSL_ROOT)/lib/libcrypto.a \
	-lGL \
	-lm \
	-lpthread \
	-ldl \
	-lrt \
	-lX11

# Windows linker flags
WIN_BASE_LDFLAGS := \
	$(WIN_RAYLIB_ROOT)/lib/libraylib.a \
	$(WIN_OPENSSL_ROOT)/lib/libssl.a \
	$(WIN_OPENSSL_ROOT)/lib/libcrypto.a \
	-Wl,--defsym,stat64i32=_stat64 \
	-lopengl32 \
	-lgdi32 \
	-lwinmm \
	-lws2_32 \
	-lcrypt32 \
	-luser32 \
	-lshell32 \
	-static-libgcc \
	-static-libstdc++

# Debug linker flags
DEBUG_LDFLAGS := \
	$(BASE_LDFLAGS)

WIN_DEBUG_LDFLAGS := \
	$(WIN_BASE_LDFLAGS)

# Release linker flags
RELEASE_LDFLAGS := \
	$(BASE_LDFLAGS) \
	-O3 \
	-flto=auto \
	-Wl,--gc-sections \
	-s

WIN_RELEASE_LDFLAGS := \
	$(WIN_BASE_LDFLAGS) \
	-Wl,--gc-sections \
	-mwindows \
	-s


.PHONY: \
	all \
	debug \
	release \
	win-debug \
	win-release \
	run \
	run-release \
	dist \
	dist-release \
	dist-win \
	dist-win-release \
	dist-release-all \
	clean \
	clear


all: debug

clear: clean

# Linux debug compilation
$(BUILD_DIR)/debug/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) \
		$(DEBUG_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

# Linux release compilation
$(BUILD_DIR)/release/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) \
		$(RELEASE_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

# Windows debug compilation
$(BUILD_DIR)/win-debug/%.o: %.cpp
	@mkdir -p $(@D)
	$(WIN_CXX) \
		$(WIN_DEBUG_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

# Windows release compilation
$(BUILD_DIR)/win-release/%.o: %.cpp
	@mkdir -p $(@D)
	$(WIN_CXX) \
		$(WIN_RELEASE_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

# Embedded Linux assets
$(RELEASE_FONT_OBJ): $(FONT_FILE)
	@mkdir -p $(@D)
	$(LD) \
		-r \
		-b binary \
		$(FONT_FILE) \
		-o $@

# Embedded Windows assets
$(WIN_RELEASE_FONT_OBJ): $(FONT_FILE)
	@mkdir -p $(@D)
	$(WIN_LD) \
		-r \
		-b binary \
		$(FONT_FILE) \
		-o $@

# Linux debug linking
debug: $(DEBUG_OBJS)
	$(CXX) \
		$(DEBUG_OBJS) \
		$(DEBUG_LDFLAGS) \
		-o $(TARGET)

# Linux release linking
release: $(RELEASE_OBJS) $(RELEASE_FONT_OBJ)
	$(CXX) \
		$(RELEASE_OBJS) \
		$(RELEASE_FONT_OBJ) \
		$(RELEASE_LDFLAGS) \
		-o $(TARGET)

# Windows debug linking
win-debug: $(WIN_DEBUG_OBJS)
	$(WIN_CXX) \
		$(WIN_DEBUG_OBJS) \
		$(WIN_DEBUG_LDFLAGS) \
		-o $(WIN_TARGET)

# Windows release linking
win-release: $(WIN_RELEASE_OBJS) $(WIN_RELEASE_FONT_OBJ)
	$(WIN_CXX) \
		$(WIN_RELEASE_OBJS) \
		$(WIN_RELEASE_FONT_OBJ) \
		$(WIN_RELEASE_LDFLAGS) \
		-o $(WIN_TARGET)

# Run
run: debug
	./$(TARGET)

run-release: release
	./$(TARGET)

# Distribution
DIST_MODE ?= debug

dist: $(DIST_MODE)
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)
	cp $(TARGET) $(DIST_DIR)/
	@echo "Linux distribution built in $(DIST_DIR)/ (mode: $(DIST_MODE))"

dist-release:
	$(MAKE) dist DIST_MODE=release

dist-win: win-debug
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)
	cp $(WIN_TARGET) $(DIST_DIR)/
	@echo "Windows distribution built in $(DIST_DIR)/ (mode: debug)"

dist-win-release: win-release
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)
	cp $(WIN_TARGET) $(DIST_DIR)/
	@echo "Windows distribution built in $(DIST_DIR)/ (mode: release)"

dist-release-all: release win-release
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)/linux
	@mkdir -p $(DIST_DIR)/windows

	cp $(TARGET) $(DIST_DIR)/linux/
	cp $(WIN_TARGET) $(DIST_DIR)/windows/

	@echo "All release distributions built:"
	@echo "  Linux:   $(DIST_DIR)/linux/$(TARGET)"
	@echo "  Windows: $(DIST_DIR)/windows/$(WIN_TARGET)"

# Dependencies
-include $(DEBUG_OBJS:.o=.d)
-include $(RELEASE_OBJS:.o=.d)
-include $(WIN_DEBUG_OBJS:.o=.d)
-include $(WIN_RELEASE_OBJS:.o=.d)

# Cleanup
clean:
	rm -f $(TARGET)
	rm -f $(WIN_TARGET)
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)
	rm -rf WFZSource/
