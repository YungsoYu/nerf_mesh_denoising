#ifndef MESH_RENDERER_H
#define MESH_RENDERER_H

#include <glad/glad.h>
#include "mesh.h"

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
    void upload(const Mesh& mesh, int highlightSelection);
    
    // Draw the mesh
    void draw() const;
    
    // Cleanup GPU resources
    void cleanup();

private:
    unsigned int VAO_ = 0;
    unsigned int VBO_ = 0;
    int vertexCount_ = 0;
    
    // Build interleaved vertex data for GPU
    std::vector<float> buildGLVertices(const Mesh& mesh, int highlightSelection) const;
};

#endif
