#ifndef UI_H
#define UI_H

#include <GLFW/glfw3.h>

struct UIState {
    // Boundary face removal: 0 = 1 edge, 1 = 2 edges
    int boundarySelection = 0;
    
    // Set true when radio button selection changes (consume in main loop)
    bool selectionChanged = false;
    
    // Action buttons (set true when clicked, consume in main loop)
    bool removeClicked = false;
    bool resetClicked = false;
};

// Initialize ImGui - call once after creating GLFW window and loading OpenGL
void initUI(GLFWwindow* window);

// Render UI panel - call every frame before glfwSwapBuffers
void renderUI(UIState& state);

// Cleanup ImGui - call before glfwTerminate
void shutdownUI();

#endif
