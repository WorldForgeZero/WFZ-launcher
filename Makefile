MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)

# Compilers
CXX := g++
WIN_CXX := x86_64-w64-mingw32-g++
WIN_WINDRES := x86_64-w64-mingw32-windres

# Project
TARGET := wfz_launcher
WIN_TARGET := wfz_launcher.exe

SRC_DIR := src
BUILD_DIR := build
DIST_DIR := dist
ASSETS_DIR := assets

# Windows resources
ICON_ICO_FILE := $(ASSETS_DIR)/icon/icon.ico

WIN_ICON_RC := \
	$(BUILD_DIR)/windows/wfz_icon.rc

WIN_DEBUG_EXE_ICON_OBJ := \
	$(BUILD_DIR)/win-debug/embedded/wfz_exe_icon.o

WIN_RELEASE_EXE_ICON_OBJ := \
	$(BUILD_DIR)/win-release/embedded/wfz_exe_icon.o

# Third-party dependencies
RMLUI_ROOT := third_party/rmlui/linux_amd64
FREETYPE_ROOT := third_party/freetype/linux_amd64
GLFW_ROOT := third_party/glfw/linux_amd64
OPENSSL_ROOT := third_party/openssl/linux_amd64

WIN_RMLUI_ROOT := third_party/rmlui/win64_mingw
WIN_FREETYPE_ROOT := third_party/freetype/win64_mingw
WIN_GLFW_ROOT := third_party/glfw/win64_mingw
WIN_OPENSSL_ROOT := third_party/openssl/win64_mingw

RMLUI_BACKEND_DIR := third_party/rmlui/backend

# Sources
PROJECT_SOURCES := \
	$(shell find $(SRC_DIR) -type f -name '*.cpp' -print)

RMLUI_BACKEND_SOURCES := \
	$(RMLUI_BACKEND_DIR)/RmlUi_Backend_GLFW_GL3.cpp \
	$(RMLUI_BACKEND_DIR)/RmlUi_Platform_GLFW.cpp \
	$(RMLUI_BACKEND_DIR)/RmlUi_Renderer_GL3.cpp

CPP_SOURCES := \
	$(PROJECT_SOURCES) \
	$(RMLUI_BACKEND_SOURCES)

# Objects
DEBUG_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/debug/%.o,$(CPP_SOURCES))

RELEASE_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/release/%.o,$(CPP_SOURCES))

WIN_DEBUG_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/win-debug/%.o,$(CPP_SOURCES))

WIN_RELEASE_OBJS := \
	$(patsubst %.cpp,$(BUILD_DIR)/win-release/%.o,$(CPP_SOURCES))

# Includes
INCLUDES := \
	-I$(SRC_DIR) \
	-I$(RMLUI_BACKEND_DIR) \
	-I$(RMLUI_ROOT)/include \
	-I$(FREETYPE_ROOT)/include/freetype2 \
	-I$(GLFW_ROOT)/include \
	-I$(OPENSSL_ROOT)/include \
	-Ithird_party

WIN_INCLUDES := \
	-I$(SRC_DIR) \
	-I$(RMLUI_BACKEND_DIR) \
	-I$(WIN_RMLUI_ROOT)/include \
	-I$(WIN_FREETYPE_ROOT)/include/freetype2 \
	-I$(WIN_GLFW_ROOT)/include \
	-I$(WIN_OPENSSL_ROOT)/include \
	-Ithird_party

# Common compiler flags
COMMON_CXXFLAGS := \
	-std=c++17 \
	-Wall \
	-Wextra \
	-pipe \
	-DRMLUI_STATIC_LIB \
	-DCPPHTTPLIB_OPENSSL_SUPPORT \
	$(INCLUDES)

WIN_COMMON_CXXFLAGS := \
	-std=c++17 \
	-Wall \
	-Wextra \
	-pipe \
	-DRMLUI_STATIC_LIB \
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
	-DNDEBUG \
	-O3 \
	-flto=auto \
	-ffunction-sections \
	-fdata-sections

WIN_RELEASE_CXXFLAGS := \
	$(WIN_COMMON_CXXFLAGS) \
	-DNDEBUG \
	-O3 \
	-flto=auto \
	-ffunction-sections \
	-fdata-sections

# Linux linker flags
BASE_LDFLAGS := \
	$(RMLUI_ROOT)/lib/librmlui.a \
	$(FREETYPE_ROOT)/lib/libfreetype.a \
	$(GLFW_ROOT)/lib/libglfw3.a \
	$(OPENSSL_ROOT)/lib/libssl.a \
	$(OPENSSL_ROOT)/lib/libcrypto.a \
	-lGL \
	-lm \
	-lpthread \
	-ldl \
	-lrt

DEBUG_LDFLAGS := \
	$(BASE_LDFLAGS)

RELEASE_LDFLAGS := \
	$(BASE_LDFLAGS) \
	-O3 \
	-flto=auto \
	-Wl,--gc-sections \
	-s

# Windows linker flags
WIN_BASE_LDFLAGS := \
	$(WIN_RMLUI_ROOT)/lib/librmlui.a \
	$(WIN_FREETYPE_ROOT)/lib/libfreetype.a \
	$(WIN_GLFW_ROOT)/lib/libglfw3.a \
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

WIN_DEBUG_LDFLAGS := \
	$(WIN_BASE_LDFLAGS)

WIN_RELEASE_LDFLAGS := \
	$(WIN_BASE_LDFLAGS) \
	-O3 \
	-flto=auto \
	-Wl,--gc-sections \
	-mwindows \
	-s

# Targets
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

# Linux compilation
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

# Windows compilation
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

# Windows executable icon
$(WIN_ICON_RC): $(ICON_ICO_FILE)
	@mkdir -p $(@D)
	@printf 'IDI_WFZ_ICON ICON "%s"\n' "$(abspath $(ICON_ICO_FILE))" > $@

$(WIN_DEBUG_EXE_ICON_OBJ): $(WIN_ICON_RC) $(ICON_ICO_FILE)
	@mkdir -p $(@D)
	$(WIN_WINDRES) \
		-i $(WIN_ICON_RC) \
		-O coff \
		-o $@

$(WIN_RELEASE_EXE_ICON_OBJ): $(WIN_ICON_RC) $(ICON_ICO_FILE)
	@mkdir -p $(@D)
	$(WIN_WINDRES) \
		-i $(WIN_ICON_RC) \
		-O coff \
		-o $@

# Linux linking
debug: $(DEBUG_OBJS)
	$(CXX) \
		$(DEBUG_OBJS) \
		$(DEBUG_LDFLAGS) \
		-o $(TARGET)

release: $(RELEASE_OBJS)
	$(CXX) \
		$(RELEASE_OBJS) \
		$(RELEASE_LDFLAGS) \
		-o $(TARGET)

# Windows linking
win-debug: \
	$(WIN_DEBUG_OBJS) \
	$(WIN_DEBUG_EXE_ICON_OBJ)

	$(WIN_CXX) \
		$(WIN_DEBUG_OBJS) \
		$(WIN_DEBUG_EXE_ICON_OBJ) \
		$(WIN_DEBUG_LDFLAGS) \
		-o $(WIN_TARGET)

win-release: \
	$(WIN_RELEASE_OBJS) \
	$(WIN_RELEASE_EXE_ICON_OBJ)

	$(WIN_CXX) \
		$(WIN_RELEASE_OBJS) \
		$(WIN_RELEASE_EXE_ICON_OBJ) \
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
	cp -r $(ASSETS_DIR) $(DIST_DIR)/
	@echo "Linux distribution built in $(DIST_DIR)/ (mode: $(DIST_MODE))"

dist-release:
	$(MAKE) dist DIST_MODE=release

dist-win: win-debug
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)
	cp $(WIN_TARGET) $(DIST_DIR)/
	cp -r $(ASSETS_DIR) $(DIST_DIR)/
	@echo "Windows distribution built in $(DIST_DIR)/ (mode: debug)"

dist-win-release: win-release
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)
	cp $(WIN_TARGET) $(DIST_DIR)/
	cp -r $(ASSETS_DIR) $(DIST_DIR)/
	@echo "Windows distribution built in $(DIST_DIR)/ (mode: release)"

dist-release-all: release win-release
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)

	cp $(TARGET) $(DIST_DIR)/
	cp $(WIN_TARGET) $(DIST_DIR)/
	cp -r $(ASSETS_DIR) $(DIST_DIR)/

	@echo "All release distributions built:"
	@echo "  Linux:   $(DIST_DIR)/$(TARGET)"
	@echo "  Windows: $(DIST_DIR)/$(WIN_TARGET)"

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
