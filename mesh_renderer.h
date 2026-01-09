#ifndef MESH_RENDERER_H
#define MESH_RENDERER_H

#include <glad/glad.h>
#include "mesh.h"

// Face color types for rendering (as float values for shader)
namespace FaceColor {
    constexpr float NORMAL = 0.0f;           // Normal face (gray)
    constexpr float BOUNDARY = 1.0f;         // Boundary face (yellow/red/orange)
    constexpr float GREEN_COMPONENT = 2.0f;  // Largest component (green)
    constexpr float BLUE_COMPONENT = 3.0f;   // 2nd largest component (blue)
}

// OpenGL renderer for Mesh (handles VBO/VAO management)
class MeshRenderer {
public:
    MeshRenderer() = default;
    ~MeshRenderer();
    
    // Disable copy (OpenGL resources)
    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer& operator=(const MeshRenderer&) = delete;
    
    // Enable move
    MeshRenderer(MeshRenderer&& other) noexcept;
    MeshRenderer& operator=(MeshRenderer&& other) noexcept;
    
    // Upload mesh data to GPU
    // highlightSelection: [0]=1 boundary edge, [1]=2 boundary edges, [2]=3 boundary edges, [3]=non-manifold faces
    void upload(const Mesh& mesh, const bool highlightSelection[4]);
    
    // Draw the mesh
    void draw() const;
    
    // Cleanup GPU resources
    void cleanup();

private:
    unsigned int VAO_ = 0;
    unsigned int VBO_ = 0;
    int vertexCount_ = 0;
    
    // Build interleaved vertex data for GPU
    std::vector<float> buildGLVertices(const Mesh& mesh, const bool highlightSelection[4]) const;
};

#endif
