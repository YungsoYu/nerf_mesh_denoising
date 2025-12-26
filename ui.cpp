#include "ui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void initUI(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    // Setup style
    ImGui::StyleColorsDark();
    
    // Setup platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void renderUI(UIState& state)
{
    // Start new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create UI panel
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(220, 160), ImGuiCond_Always);
    ImGui::Begin("Boundary Face Removal", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Radio buttons for boundary selection
    ImGui::Text("Select faces to remove:");
    if (ImGui::RadioButton("1 Boundary Edge", &state.boundarySelection, 0)) {
        state.selectionChanged = true;
    }
    if (ImGui::RadioButton("2 Boundary Edges", &state.boundarySelection, 1)) {
        state.selectionChanged = true;
    }
    if (ImGui::RadioButton("3 Boundary Edges", &state.boundarySelection, 2)) {
        state.selectionChanged = true;
    }
    
    ImGui::Spacing();
    
    // Action buttons
    if (ImGui::Button("Remove")) {
        state.removeClicked = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        state.resetClicked = true;
    }

    ImGui::End();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void shutdownUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
