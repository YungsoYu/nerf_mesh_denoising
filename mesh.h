#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <utility>

// Edge type: ordered pair of vertex indices (smaller index first)
using Edge = std::pair<int, int>;

// Indexed mesh representation
struct Mesh {
    // Geometry data
    std::vector<glm::vec3> vertices;      // Unique vertex positions
    std::vector<int> indices;             // 3 indices per triangle
    std::vector<glm::vec3> faceNormals;   // One normal per triangle
    std::map<Edge, std::vector<int>> edgeToFaces; // Edge - adjacent faces mapping
    
    std::vector<int> boundaryFaces_1;
    std::vector<int> boundaryFaces_2;
    std::vector<int> boundaryFaces_3;

    // OpenGL
    std::vector<float> glVertices;        // Interleaved data for GPU
    unsigned int VAO = 0, VBO = 0;
    int vertexCount = 0;
};

// Create an edge key with consistent ordering
inline Edge makeEdge(int v0, int v1) {
    return v0 < v1 ? Edge{v0, v1} : Edge{v1, v0};
}

// Load OBJ file into indexed mesh.
Mesh loadOBJ(const std::string& path);

// Build edge - adjacent faces map 
std::map<Edge, std::vector<int>> buildEdgeFaceAdjacency(const Mesh& mesh);

// Count mesh edge
void analyzeMesh(const std::map<Edge, std::vector<int>>& mesh);

void findBoundaryFaces(Mesh& mesh);

// Remove boundary faces from mesh (selection: 0 = 1 edge, 1 = 2 edges, 2 = 3 edges)
void removeBoundaryFaces(Mesh& mesh, int boundarySelection);

// Build OpenGL VBO from mesh and upload to GPU.
// highlightSelection: -1 = no highlight, 0/1/2 = highlight 1/2/3-edge faces
void prepareMeshForGL(Mesh& mesh, int highlightSelection = -1);

#endif
