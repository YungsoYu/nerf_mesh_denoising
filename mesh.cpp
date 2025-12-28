#include "mesh.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// Hash for glm::vec3 (exact coordinate matching)
struct Vec3Hash {
    size_t operator()(const glm::vec3& v) const {
        size_t h1 = std::hash<float>()(v.x);
        size_t h2 = std::hash<float>()(v.y);
        size_t h3 = std::hash<float>()(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct Vec3Equal {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

Mesh Mesh::fromOBJ(const std::string& path)
{
    Mesh mesh;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << path << std::endl;
        return mesh;
    }
    
    // vertex position - index pair 
    std::unordered_map<glm::vec3, int, Vec3Hash, Vec3Equal> verticesMap;
    
    // index mapping from old (duplicated vertices in original file) to new (unique vertices in verticesMap)
    std::vector<int> oldToNewIndex;
    
    int rawVertexCount = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            // Parse vertex position
            std::istringstream iss(line.substr(2));
            float x, y, z;
            iss >> x >> y >> z;
            glm::vec3 pos(x, y, z);
            
            // Check if this position already exists
            auto it = verticesMap.find(pos);
            if (it == verticesMap.end()) {
                // New unique vertex
                int newIdx = static_cast<int>(mesh.vertices_.size());
                verticesMap[pos] = newIdx;
                mesh.vertices_.push_back(pos);
                oldToNewIndex.push_back(newIdx);
            } else {
                // Duplicate
                oldToNewIndex.push_back(it->second);
            }
            rawVertexCount++;
        } 
        else if (line.substr(0, 2) == "f ") {
            // Parse triangle face
            std::istringstream iss(line.substr(2));
            std::string token;
            int indices[3];
            
            for (int i = 0; i < 3 && iss >> token; ++i) {
                size_t pos = token.find('/');
                if (pos != std::string::npos) {
                    token = token.substr(0, pos);
                }
                int objIdx = std::stoi(token) - 1;
                indices[i] = oldToNewIndex[objIdx];
            }
            
            mesh.indices_.push_back(indices[0]);
            mesh.indices_.push_back(indices[1]);
            mesh.indices_.push_back(indices[2]);
            
            // Compute face normal
            glm::vec3 v0 = mesh.vertices_[indices[0]];
            glm::vec3 v1 = mesh.vertices_[indices[1]];
            glm::vec3 v2 = mesh.vertices_[indices[2]];
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            
            mesh.faceNormals_.push_back(normal);
        }
    }
    
    std::cout << "Loaded OBJ file and removed duplicates:\n"
              << "  Vertices including duplicates: " << rawVertexCount << "\n"
              << "  Vertices excluding duplicates: " << mesh.vertices_.size() << "\n"
              << "  Triangles: " << mesh.indices_.size() / 3 << std::endl;

    mesh.buildEdgeFaceAdjacency();
    mesh.analyzeMesh();
    mesh.findBoundaryFaces();

    return mesh;
}

void Mesh::buildEdgeFaceAdjacency() {
    edgeToFaces_.clear();
    
    size_t numTriangles = indices_.size() / 3;
    for (size_t faceIdx = 0; faceIdx < numTriangles; ++faceIdx) {
        int v0 = indices_[faceIdx * 3 + 0];
        int v1 = indices_[faceIdx * 3 + 1];
        int v2 = indices_[faceIdx * 3 + 2];
        
        edgeToFaces_[makeEdge(v0, v1)].push_back(faceIdx);
        edgeToFaces_[makeEdge(v1, v2)].push_back(faceIdx);
        edgeToFaces_[makeEdge(v2, v0)].push_back(faceIdx);
    }
}

void Mesh::analyzeMesh() const {
    int boundaryEdges = 0;
    int manifoldEdges = 0;
    int nonManifoldEdges_3 = 0;
    int nonManifoldEdges_4 = 0;
    int nonManifoldEdges = 0;

    for (const auto& [edge, faceList] : edgeToFaces_) {
        size_t count = faceList.size();
        
        if (count == 1) {
            boundaryEdges++;
        } else if (count == 2) {
            manifoldEdges++;
        } else if (count == 3) {
            nonManifoldEdges_3++;
        } else if (count == 4) {
            nonManifoldEdges_4++;
        } else {
            nonManifoldEdges++;
        }
    }

    std::cout << "Edge analysis:\n"
              << "  Total edges: " << edgeToFaces_.size() << "\n"
              << "  Boundary edges (1 face): " << boundaryEdges << "\n"
              << "  Manifold edges (2 faces): " << manifoldEdges << "\n"
              << "  Non-manifold edges (3 faces): " << nonManifoldEdges_3 << "\n"
              << "  Non-manifold edges (4 faces): " << nonManifoldEdges_4 << "\n"
              << "  Non-manifold edges (5+ faces): " << nonManifoldEdges << std::endl;
}

void Mesh::findBoundaryFaces() {
    for (auto& vec : boundaryFaces_) {
        vec.clear();
    }
    
    int numTriangles = indices_.size() / 3;
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {
        int numBoundaryEdges = 0;
        for (int i = 0; i < 3; i++) {
            int v0 = indices_[faceIdx * 3 + i];
            int v1 = indices_[faceIdx * 3 + (i + 1) % 3];
            if (edgeToFaces_.at(makeEdge(v0, v1)).size() == 1) {
                numBoundaryEdges++;
            }
        }
        if (numBoundaryEdges >= 1 && numBoundaryEdges <= 3) {
            boundaryFaces_[numBoundaryEdges - 1].push_back(faceIdx);
        }
    }
    
    std::cout << "Boundary faces:\n"
              << "  1 boundary edge: " << boundaryFaces_[0].size() << "\n"
              << "  2 boundary edges: " << boundaryFaces_[1].size() << "\n"
              << "  3 boundary edges: " << boundaryFaces_[2].size() << std::endl;
}

void Mesh::removeBoundaryFaces(int boundarySelection) {
    if (boundarySelection < 0 || boundarySelection > 2) {
        return;
    }
    
    const std::vector<int>& facesToRemoveVec = boundaryFaces_[boundarySelection];
    
    if (facesToRemoveVec.empty()) {
        return;
    }
    
    // Build set for fast lookup
    std::unordered_set<int> facesToRemove(facesToRemoveVec.begin(), facesToRemoveVec.end());
    
    // Filter faces - skip those in the removal set
    std::vector<int> newIndices;
    std::vector<glm::vec3> newNormals;
    
    int numTriangles = indices_.size() / 3;
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {
        if (facesToRemove.count(faceIdx) == 0) {
            // Keep this face
            newIndices.push_back(indices_[faceIdx * 3 + 0]);
            newIndices.push_back(indices_[faceIdx * 3 + 1]);
            newIndices.push_back(indices_[faceIdx * 3 + 2]);
            newNormals.push_back(faceNormals_[faceIdx]);
        }
    }
    
    std::cout << "Removed " << facesToRemove.size() << " boundary faces" << std::endl;
    
    indices_ = std::move(newIndices);
    faceNormals_ = std::move(newNormals);
    
    // Rebuild adjacency and boundary info after modification
    buildEdgeFaceAdjacency();
    analyzeMesh();
    findBoundaryFaces();
}
