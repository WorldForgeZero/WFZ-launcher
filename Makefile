MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)

CXX := g++
WIN_CXX := x86_64-w64-mingw32-g++

LD := ld
WIN_LD := x86_64-w64-mingw32-ld

SRC_DIR := src
BUILD_DIR := build
DIST_DIR := dist

TARGET := wfz_launcher
WIN_TARGET := wfz_launcher.exe

FONT_FILE := assets/fonts/Monocraft.ttf

RAYLIB_ROOT := third_party/raylib/linux_amd64
OPENSSL_ROOT := third_party/openssl/linux_amd64

WIN_RAYLIB_ROOT := third_party/raylib/win64_mingw
WIN_OPENSSL_ROOT := third_party/openssl/win64_mingw

CPP_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.cpp' -print)

DEBUG_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/debug/%.o,$(CPP_SOURCES))

RELEASE_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/release/%.o,$(CPP_SOURCES))

WIN_DEBUG_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/win-debug/%.o,$(CPP_SOURCES))

WIN_RELEASE_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/win-release/%.o,$(CPP_SOURCES))

RELEASE_FONT_OBJ := \
	$(BUILD_DIR)/release/embedded/monocraft_font.o

WIN_RELEASE_FONT_OBJ := \
	$(BUILD_DIR)/win-release/embedded/monocraft_font.o

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

COMMON_CXXFLAGS := \
	-std=c++17 \
	-Wall \
	-Wextra \
	-DCPPHTTPLIB_OPENSSL_SUPPORT \
	$(INCLUDES)

WIN_COMMON_CXXFLAGS := \
	-std=c++17 \
	-Wall \
	-Wextra \
	-DCPPHTTPLIB_OPENSSL_SUPPORT \
	$(WIN_INCLUDES)

DEBUG_CXXFLAGS := \
	$(COMMON_CXXFLAGS) \
	-g \
	-O0

WIN_DEBUG_CXXFLAGS := \
	$(WIN_COMMON_CXXFLAGS) \
	-g \
	-O0

RELEASE_CXXFLAGS := \
	$(COMMON_CXXFLAGS) \
	-DEMBEDED_FONT \
	-DNDEBUG \
	-Os \
	-flto \
	-ffunction-sections \
	-fdata-sections

WIN_RELEASE_CXXFLAGS := \
	$(WIN_COMMON_CXXFLAGS) \
	-DEMBEDED_FONT \
	-DNDEBUG \
	-Os \
	-flto \
	-ffunction-sections \
	-fdata-sections

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

DEBUG_LDFLAGS := \
	$(BASE_LDFLAGS)

WIN_DEBUG_LDFLAGS := \
	$(WIN_BASE_LDFLAGS)

RELEASE_LDFLAGS := \
	$(BASE_LDFLAGS) \
	-flto \
	-Wl,--gc-sections \
	-s

WIN_RELEASE_LDFLAGS := \
	$(WIN_BASE_LDFLAGS) \
	-flto \
	-Wl,--gc-sections \
	-s \
	-mwindows


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
	clean \
	clear


all: debug

clear: clean

# Compilation
$(BUILD_DIR)/debug/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) \
		$(DEBUG_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(BUILD_DIR)/release/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) \
		$(RELEASE_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(BUILD_DIR)/win-debug/%.o: %.cpp
	@mkdir -p $(@D)
	$(WIN_CXX) \
		$(WIN_DEBUG_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(BUILD_DIR)/win-release/%.o: %.cpp
	@mkdir -p $(@D)
	$(WIN_CXX) \
		$(WIN_RELEASE_CXXFLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

# Embedded assets
$(RELEASE_FONT_OBJ): $(FONT_FILE)
	@mkdir -p $(@D)
	$(LD) \
		-r \
		-b binary \
		$(FONT_FILE) \
		-o $@

$(WIN_RELEASE_FONT_OBJ): $(FONT_FILE)
	@mkdir -p $(@D)
	$(WIN_LD) \
		-r \
		-b binary \
		$(FONT_FILE) \
		-o $@

# Linking
debug: $(DEBUG_OBJS)
	$(CXX) \
		$(DEBUG_OBJS) \
		$(DEBUG_LDFLAGS) \
		-o $(TARGET)

release: $(RELEASE_OBJS) $(RELEASE_FONT_OBJ)
	$(CXX) \
		$(RELEASE_OBJS) \
		$(RELEASE_FONT_OBJ) \
		$(RELEASE_LDFLAGS) \
		-o $(TARGET)

win-debug: $(WIN_DEBUG_OBJS)
	$(WIN_CXX) \
		$(WIN_DEBUG_OBJS) \
		$(WIN_DEBUG_LDFLAGS) \
		-o $(WIN_TARGET)

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
