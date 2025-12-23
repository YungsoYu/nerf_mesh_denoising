#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

// Indexed mesh representation
struct Mesh {
    // Geometry data
    std::vector<glm::vec3> vertices;      // Unique vertex positions
    std::vector<int> indices;             // 3 indices per triangle
    std::vector<glm::vec3> faceNormals;   // One normal per triangle
    
    // OpenGL
    std::vector<float> glVertices;        // Interleaved data for GPU
    unsigned int VAO = 0, VBO = 0;
    int vertexCount = 0;
};

// Load OBJ file into indexed mesh.
Mesh loadOBJ(const std::string& path);

// Build OpenGL VBO from mesh and upload to GPU.
void prepareMeshForGL(Mesh& mesh);

#endif
