#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <array>
#include <utility>

// Edge type: ordered pair of vertex indices (smaller index first)
using Edge = std::pair<int, int>;

// Create an edge key with consistent ordering
inline Edge makeEdge(int v0, int v1) {
    return v0 < v1 ? Edge{v0, v1} : Edge{v1, v0};
}

// Indexed mesh representation (geometry data only)
class Mesh {
public:
    // Factory method to load from OBJ file
    static Mesh fromOBJ(const std::string& path);
    
    // Mesh operations
    void removeBoundaryFaces(int boundarySelection);
    
    // Getters
    const std::vector<glm::vec3>& vertices() const { return vertices_; }
    const std::vector<int>& indices() const { return indices_; }
    const std::vector<glm::vec3>& faceNormals() const { return faceNormals_; }
    const std::vector<int>& boundaryFaces(int selection) const { return boundaryFaces_[selection]; }
    size_t faceCount() const { return indices_.size() / 3; }

private:
    // Geometry data
    std::vector<glm::vec3> vertices_;      // Unique vertex positions
    std::vector<int> indices_;             // 3 indices per triangle
    std::vector<glm::vec3> faceNormals_;   // One normal per triangle
    std::map<Edge, std::vector<int>> edgeToFaces_; // Edge - adjacent faces mapping
    
    // Boundary faces by edge count: [0]=1 edge, [1]=2 edges, [2]=3 edges
    std::array<std::vector<int>, 3> boundaryFaces_;
    
    // Internal helpers
    void buildEdgeFaceAdjacency();
    void findBoundaryFaces();
    void analyzeMesh() const;
};

#endif
