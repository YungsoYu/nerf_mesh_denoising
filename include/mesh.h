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
    
    std::vector<std::vector<int>> getComponents();

    // Mesh operations
    void removeFaces(const std::vector<int>& facesToRemove);
    void removeBoundaryFaces(int boundarySelection);  
    void removeNonManifoldFaces();
    void removeRestComponents(std::vector<std::vector<int>>& components);

    
    void buildEdgeFaceAdjacency();
    void analyzeMesh() const;
    void buildValence();
    void buildFaceStatus();
    void findBoundaryFaces();
    void findNonManifoldFacesToRemove();
    
    // Getters
    const std::vector<glm::vec3>& vertices() const { return vertices_; }
    const std::vector<int>& indices() const { return indices_; }
    const std::vector<glm::vec3>& faceNormals() const { return faceNormals_; }
    const std::vector<float>& faceColors() const { return faceColors_; }
    const std::vector<int>& faceBoundaryCount() const { return faceBoundaryCount_; }
    const std::vector<int>& faceManifoldCount() const { return faceManifoldCount_; }
    const std::vector<int>& faceNonManifoldCount() const { return faceNonManifoldCount_; }

    size_t faceCount() const { return indices_.size() / 3; }
    const std::vector<int>& vertexValences() const { return vertexValences_; }

    int numEdgesWith3Faces() const { return numEdgesWith3Faces_; }
    int numEdgesWith4Faces() const { return numEdgesWith4Faces_; }

private:
    // Geometry data
    std::vector<glm::vec3> vertices_;      // Unique vertex positions
    std::vector<int> indices_;             // 3 indices per triangle
    std::vector<glm::vec3> faceNormals_;   // One normal per triangle

    std::vector<float> faceColors_;             
    std::vector<int> faceBoundaryCount_;    
    std::vector<int> faceManifoldCount_;          
    std::vector<int> faceNonManifoldCount_; // Not used currently

    std::map<Edge, std::vector<int>> edgeToFaces_; // Edge - adjacent faces mapping
    std::vector<int> vertexValences_; // Vertex valence: number of neighboring vertices for each vertex

    mutable int numEdgesWith3Faces_;  // Cache for analysis 
    mutable int numEdgesWith4Faces_;  // Cache for analysis 

    // Load OBJ file to the mesh
    void loadFromOBJ(const std::string& path);
    void resetFaceData(); 
    
};

#endif
