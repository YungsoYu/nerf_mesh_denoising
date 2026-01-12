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

    // Create UI panel (width reduced to 3/4: 250 * 0.75 = 187.5 ≈ 188)
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(188, 350), ImGuiCond_Always);
    ImGui::Begin("Mesh", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // === Mesh Rendering ===
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Mesh Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Transparent Face", &state.transparentFace);

    }
    
    // === Auto Mesh Cleanup ===
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Auto Mesh Cleanup", ImGuiTreeNodeFlags_DefaultOpen)) {

    }
    
    // === Manual Refinement ===
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Manual Mesh Cleanup", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("Select faces to highlight:");
        if (ImGui::RadioButton("1 Boundary Edge", &state.boundarySelection, 0)) {
            state.selectionChanged = true;
        }
        if (ImGui::RadioButton("2 Boundary Edges", &state.boundarySelection, 1)) {
            state.selectionChanged = true;
        }
        // Set text wrap position for long labels
        ImGui::PushTextWrapPos(ImGui::GetContentRegionMax().x);
        if (ImGui::RadioButton("Non-manifold faces to remove", &state.boundarySelection, 2)) {
            state.selectionChanged = true;
        }
        ImGui::PopTextWrapPos();
        
        ImGui::Spacing();
        // Button - use short text to fit in smaller width
        if (ImGui::Button("Remove highlighted faces")) {
            state.removeClicked = true;
        }

    }
    
    // === Smoothing ===
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Smoothing", ImGuiTreeNodeFlags_DefaultOpen)) {

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
