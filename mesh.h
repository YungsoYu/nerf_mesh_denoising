#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <array>
#include <utility>
#include <unordered_set>

// Edge type: ordered pair of vertex indices (smaller index first)
using Edge = std::pair<int, int>;

// Create an edge key with consistent ordering
inline Edge makeEdge(int v0, int v1) {
    return v0 < v1 ? Edge{v0, v1} : Edge{v1, v0};
}

// Indexed mesh representation (geometry data only)
class Mesh {
public:
    // Factory method to load from directory containing OBJ files (creates new mesh)
    static Mesh createMesh(const std::string& meshDir);
    // Factory method to combine two components into a new mesh
    static Mesh fromComponents(const Mesh& source,
                               const std::vector<int>& component0, 
                               const std::vector<int>& component1);
    
    
    // Mesh operations
    void removeFaces(const std::vector<int>& facesToRemove);
    void removeBoundaryFaces(int boundarySelection);  // Convenience wrapper
    void removeNonManifoldFaces();  // Remove faces stored in nonManifoldToRemove
    void analyzeComponents() const;
    // Keep only the two largest components (removes all other components)
    void keepLargestTwoComponents();
    
    // Analysis functions (called from main)
    void buildEdgeFaceAdjacency();
    void findBoundaryFaces();
    void analyzeMesh() const;
    void buildValence();
    
    // Getters
    const std::vector<glm::vec3>& vertices() const { return vertices_; }
    const std::vector<int>& indices() const { return indices_; }
    const std::vector<glm::vec3>& faceNormals() const { return faceNormals_; }
    const std::vector<int>& boundaryFaces(int selection) const { return boundaryFaces_[selection]; }
    const std::vector<std::vector<int>>& components() const { return components_; }
    const std::vector<int>& vertexValences() const { return vertexValences_; }
    size_t faceCount() const { return indices_.size() / 3; }

     std::unordered_set<int> nonManifoldToRemove;  // Set of valid faces to render
    void findNonManifoldFacesToRemove();
    
    // Helper function to check if a face is valid (customize this based on your criteria)
    bool isFaceValid(int faceIdx) const;

private:
    // Geometry data
    std::vector<glm::vec3> vertices_;      // Unique vertex positions
    std::vector<int> indices_;             // 3 indices per triangle
    std::vector<glm::vec3> faceNormals_;   // One normal per triangle
    std::map<Edge, std::vector<int>> edgeToFaces_; // Edge - adjacent faces mapping
    
    // Boundary faces by edge count: [0]=1 edge, [1]=2 edges, [2]=3 edges
    std::array<std::vector<int>, 3> boundaryFaces_;
    
    // Connected components (sorted by size): each component is a vector of face indices
    mutable std::vector<std::vector<int>> components_;
    
    // Vertex valence: number of neighboring vertices for each vertex
    std::vector<int> vertexValences_;

    // Load OBJ file and append to existing mesh
    void loadFromOBJ(const std::string& path);
    
};

#endif
