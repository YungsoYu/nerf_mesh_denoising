#ifndef UI_H
#define UI_H

#include <GLFW/glfw3.h>

struct UIState {
    // Face selection: -1 = none, 0 = 1 boundary edge, 1 = 2 boundary edges, 2 = non-manifold faces to remove
    int boundarySelection = -1;
    
    // Transparent face rendering
    bool transparentFace = false;
    
    // Set true when radio button selection changes (consume in main loop)
    bool selectionChanged = false;
    
    // Action buttons (set true when clicked, consume in main loop)
    bool removeClicked = false;
    bool resetClicked = false;
    bool traverseClicked = false;
    bool traverse2Clicked = false;
    bool componentClicked = false;
};

// Initialize ImGui - call once after creating GLFW window and loading OpenGL
void initUI(GLFWwindow* window);

// Render UI panel - call every frame before glfwSwapBuffers
void renderUI(UIState& state);

// Cleanup ImGui - call before glfwTerminate
void shutdownUI();

#endif
