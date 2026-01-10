#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>

#include "mesh.h"
#include "mesh_renderer.h"
#include "shader.h"
#include "ui.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
float orbitRadius = 5.0f;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  orbitRadius);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool firstMouse = true;
bool dragging = false;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = 90.0f;
float pitch = 0.0f;
float fov   =  45.0f;


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OBJ Viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Register mouse callbacks
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, mouse_callback); 
    glfwSetScrollCallback(window, scroll_callback); 

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize UI
    initUI(window);
    UIState uiState;

    // Shader
    std::string vertexShaderSource = loadShaderFile("shader.vert");
    std::string fragmentShaderSource = loadShaderFile("shader.frag");
    
    const char* vertexShaderCode = vertexShaderSource.c_str();
    const char* fragmentShaderCode = fragmentShaderSource.c_str();
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Collect all OBJ file paths
    std::vector<std::string> objPaths;
    std::vector<Mesh> meshes;
    std::vector<Mesh> originalMeshes;  // Store originals for reset
    std::vector<MeshRenderer> renderers;
    std::string meshDir = "mesh/hotdog/";
    
    DIR* dir = opendir(meshDir.c_str());
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".obj") {
            objPaths.push_back(meshDir + filename);
            // For rendering, we still need individual meshes
            // meshes.push_back(Mesh::fromOBJ(meshDir + filename));
            // renderers.emplace_back();
        }
    }
    closedir(dir);

    // Load all OBJ files into a single combined mesh
    Mesh mesh;
    if (!objPaths.empty()) {
        // mesh = Mesh::fromOBJ(objPaths);

        mesh = Mesh::createMesh(meshDir);
        
        mesh.buildEdgeFaceAdjacency();
        std::vector<std::vector<int>> components = mesh.getComponents(); 
        if (components.size() > 1) {
            mesh.removeRestComponents(components);
        } else {
            mesh.analyzeMesh();
            mesh.findBoundaryFaces();
            mesh.buildValence();
            mesh.findNonManifoldFacesToRemove();
        }
    }

    // Create renderer for combined mesh (to show component highlighting)
    MeshRenderer meshRenderer;
    if (!objPaths.empty()) {
        // Convert int to bool array for renderer
        bool boundarySelectionArray[4] = {false, false, false, false};
        if (uiState.boundarySelection >= 0 && uiState.boundarySelection < 4) {
            boundarySelectionArray[uiState.boundarySelection] = true;
        }
        meshRenderer.upload(mesh, boundarySelectionArray);
    }


    // Matrices and uniform locations
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "modelMatrix");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "viewMatrix");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projectionMatrix");
    
    // Lighting uniform locations
    unsigned int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    unsigned int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    unsigned int boundaryColorLoc = glGetUniformLocation(shaderProgram, "boundaryColor");
    unsigned int alphaLoc = glGetUniformLocation(shaderProgram, "alpha");

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // Handle remove button click
        if (uiState.removeClicked) {
            uiState.removeClicked = false;
            
            // Remove faces based on selection
            if (!objPaths.empty() && uiState.boundarySelection >= 0) {
                if (uiState.boundarySelection < 3) {
                    // Remove boundary faces
                    mesh.removeBoundaryFaces(uiState.boundarySelection);
                } else if (uiState.boundarySelection == 3) {
                    // Remove non-manifold faces
                    mesh.removeNonManifoldFaces();
                }
                // Convert int to bool array for renderer
                bool boundarySelectionArray[4] = {false, false, false, false};
                if (uiState.boundarySelection >= 0 && uiState.boundarySelection < 4) {
                    boundarySelectionArray[uiState.boundarySelection] = true;
                }
                meshRenderer.upload(mesh, boundarySelectionArray);
            }
            
        }
        
        // Handle selection change
        if (uiState.selectionChanged) {
            uiState.selectionChanged = false;
            if (!objPaths.empty()) {
                // Convert int to bool array for renderer
                bool boundarySelectionArray[4] = {false, false, false, false};
                if (uiState.boundarySelection >= 0 && uiState.boundarySelection < 4) {
                    boundarySelectionArray[uiState.boundarySelection] = true;
                }
                meshRenderer.upload(mesh, boundarySelectionArray);
            }
        }
        
        
        // Handle traverse button click
        if (uiState.traverseClicked) {
            uiState.traverseClicked = false;
            if (!objPaths.empty()) {
                mesh.findNonManifoldFacesToRemove();
                // Update renderer to reflect marked faces
                bool boundarySelectionArray[4] = {false, false, false, false};
                if (uiState.boundarySelection >= 0 && uiState.boundarySelection < 4) {
                    boundarySelectionArray[uiState.boundarySelection] = true;
                }
                meshRenderer.upload(mesh, boundarySelectionArray);
            }
        }
        
        
        // Handle component button click
        if (uiState.componentClicked) {
            uiState.componentClicked = false;
            if (!objPaths.empty()) {
                mesh.buildEdgeFaceAdjacency();
                mesh.findBoundaryFaces();
                mesh.buildValence();
                bool boundarySelectionArray[4] = {false, false, false, false};
                if (uiState.boundarySelection >= 0 && uiState.boundarySelection < 4) {
                    boundarySelectionArray[uiState.boundarySelection] = true;
                }
                meshRenderer.upload(mesh, boundarySelectionArray);
            }
        }

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 projectionMatrix = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Pass matrices to shader
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        
        // Pass lighting uniforms
        glm::vec3 lightPos(0.0f, 20.0f, 0.0f);
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
        
        // Object color and boundary highlight color
        glm::vec3 objectColor(0.9f, 0.9f, 0.9f);
        glm::vec3 boundaryColor(1.0f, 0.9f, 0.2f); // Default: Yellow
        
        // Use color based on selected boundary type
        if (uiState.boundarySelection == 0) {
            boundaryColor = glm::vec3(1.0f, 0.9f, 0.2f); // Yellow for 1 boundary edge
        } else if (uiState.boundarySelection == 1) {
            boundaryColor = glm::vec3(1.0f, 0.3f, 0.3f); // Red for 2 boundary edges
        } else if (uiState.boundarySelection == 2) {
            boundaryColor = glm::vec3(1.0f, 0.5f, 0.0f); // Orange for 3 boundary edges
        } else if (uiState.boundarySelection == 3) {
            boundaryColor = glm::vec3(0.0f, 1.0f, 0.0f); // Green for non-manifold faces
        } else if (uiState.boundarySelection == 3) {
            boundaryColor = glm::vec3(0.0f, 1.0f, 0.0f); // Green for non-manifold faces
        }
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(objectColor));
        glUniform3fv(boundaryColorLoc, 1, glm::value_ptr(boundaryColor));
        
        // Alpha value for transparency
        float alpha = uiState.transparentFace ? 0.5f : 1.0f;
        glUniform1f(alphaLoc, alpha);

        // Draw combined mesh (with component highlighting)
        if (!objPaths.empty()) {
            meshRenderer.draw();
        } else {
            // Fallback to individual meshes if no combined mesh
            for (const auto& renderer : renderers) {
                renderer.draw();
            }
        }

        // Render UI
        renderUI(uiState);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    shutdownUI();
    renderers.clear();  // MeshRenderer destructor handles VAO/VBO cleanup
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // Check left button state to decide when to orbit
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state != GLFW_PRESS)
    {
        dragging = false;
        return;
    }
    if (!dragging)
    {
        dragging = true;
        firstMouse = true; 
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
  
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    // Orbit
    cameraPos.x = orbitRadius * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraPos.y = orbitRadius * sin(glm::radians(pitch));
    cameraPos.z = orbitRadius * sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    // Look at origin
    cameraFront = glm::normalize(-cameraPos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f; 
}
