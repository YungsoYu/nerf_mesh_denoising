CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Iinclude -Idependencies/include -Idependencies/imgui
LDFLAGS := -Ldependencies/library -lglfw -lOpenMeshCore -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -Wl,-rpath,@executable_path/../dependencies/library

TARGET_DIR := build
TARGET := $(TARGET_DIR)/opengl_app

SRC := src/main.cpp src/mesh.cpp src/mesh_renderer.cpp src/shader.cpp src/ui.cpp
# ImGui source files
SRC += dependencies/imgui/imgui.cpp dependencies/imgui/imgui_draw.cpp dependencies/imgui/imgui_tables.cpp dependencies/imgui/imgui_widgets.cpp
SRC += dependencies/imgui/imgui_impl_glfw.cpp dependencies/imgui/imgui_impl_opengl3.cpp
# Use glad.c from include directory
GLAD_SRC := $(wildcard include/glad.c)
SRC += $(GLAD_SRC)

.PHONY: all run clean info

all: info $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(TARGET_DIR)

info:
	@if [ ! -f "dependencies/library/libglfw.3.4.dylib" ]; then \
		echo "Warning: dependencies/library/libglfw.3.4.dylib not found."; \
		echo "Place your GLFW dylib at dependencies/library/ or install via Homebrew and adjust LDFLAGS."; \
	fi
	@if [ ! -f "include/glad.c" ]; then \
		echo "Note: GLAD source not found (include/glad.c)."; \
		echo "Add your generated glad.c (matching the glad headers in dependencies/include)."; \
	fi
