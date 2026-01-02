#include "mesh.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

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

    return mesh;
}

Mesh Mesh::fromOBJ(const std::vector<std::string>& paths) {
    Mesh mesh;
    
    if (paths.empty()) {
        return mesh;
    }
    
    // Load first file
    mesh = fromOBJ(paths[0]);
    
    // Load remaining files and append
    for (size_t i = 1; i < paths.size(); ++i) {
        mesh.loadFromOBJ(paths[i]);
    }
    
    return mesh;
}

Mesh Mesh::fromComponents(const Mesh& source,
                          const std::vector<int>& component0, 
                          const std::vector<int>& component1) {
    Mesh combinedMesh;
    
    // Combine both components into a set for fast lookup
    std::unordered_set<int> facesToInclude;
    facesToInclude.insert(component0.begin(), component0.end());
    facesToInclude.insert(component1.begin(), component1.end());
    
    // Get source mesh data
    const auto& sourceVertices = source.vertices();
    const auto& sourceIndices = source.indices();
    const auto& sourceFaceNormals = source.faceNormals();
    
    // Map from old vertex index to new vertex index
    std::unordered_map<int, int> oldToNewVertexMap;
    
    // First pass: collect all vertices used by the selected faces
    for (int faceIdx : facesToInclude) {
        if (faceIdx < 0 || faceIdx >= static_cast<int>(source.faceCount())) {
            continue; // Skip invalid face indices
        }
        
        // Get the three vertex indices for this face
        int v0 = sourceIndices[faceIdx * 3 + 0];
        int v1 = sourceIndices[faceIdx * 3 + 1];
        int v2 = sourceIndices[faceIdx * 3 + 2];
        
        // Add vertices to the map if not already present
        if (oldToNewVertexMap.find(v0) == oldToNewVertexMap.end()) {
            int newIdx = static_cast<int>(combinedMesh.vertices_.size());
            oldToNewVertexMap[v0] = newIdx;
            combinedMesh.vertices_.push_back(sourceVertices[v0]);
        }
        if (oldToNewVertexMap.find(v1) == oldToNewVertexMap.end()) {
            int newIdx = static_cast<int>(combinedMesh.vertices_.size());
            oldToNewVertexMap[v1] = newIdx;
            combinedMesh.vertices_.push_back(sourceVertices[v1]);
        }
        if (oldToNewVertexMap.find(v2) == oldToNewVertexMap.end()) {
            int newIdx = static_cast<int>(combinedMesh.vertices_.size());
            oldToNewVertexMap[v2] = newIdx;
            combinedMesh.vertices_.push_back(sourceVertices[v2]);
        }
    }
    
    // Second pass: build indices and normals for the combined mesh
    for (int faceIdx : facesToInclude) {
        if (faceIdx < 0 || faceIdx >= static_cast<int>(source.faceCount())) {
            continue;
        }
        
        // Get old vertex indices
        int oldV0 = sourceIndices[faceIdx * 3 + 0];
        int oldV1 = sourceIndices[faceIdx * 3 + 1];
        int oldV2 = sourceIndices[faceIdx * 3 + 2];
        
        // Map to new vertex indices
        int newV0 = oldToNewVertexMap[oldV0];
        int newV1 = oldToNewVertexMap[oldV1];
        int newV2 = oldToNewVertexMap[oldV2];
        
        // Add indices
        combinedMesh.indices_.push_back(newV0);
        combinedMesh.indices_.push_back(newV1);
        combinedMesh.indices_.push_back(newV2);
        
        // Add face normal
        combinedMesh.faceNormals_.push_back(sourceFaceNormals[faceIdx]);
    }
    
    std::cout << "Combined components into new mesh:\n"
              << "  Component 0 faces: " << component0.size() << "\n"
              << "  Component 1 faces: " << component1.size() << "\n"
              << "  Total faces: " << facesToInclude.size() << "\n"
              << "  Vertices: " << combinedMesh.vertices_.size() << "\n"
              << "  Triangles: " << combinedMesh.faceCount() << std::endl;
    
    // Build edge-face adjacency for the new mesh (needed for boundary analysis)
    combinedMesh.buildEdgeFaceAdjacency();
    
    return combinedMesh;
}

void Mesh::loadFromOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << path << std::endl;
        return;
    }
    
    // Store current vertex count to offset indices
    int vertexOffset = static_cast<int>(vertices_.size());
    
    // vertex position - index pair (local to this file)
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
            
            // Check if this position already exists in local map
            auto it = verticesMap.find(pos);
            if (it == verticesMap.end()) {
                // New unique vertex - add to mesh
                int newIdx = static_cast<int>(vertices_.size());
                verticesMap[pos] = newIdx;
                vertices_.push_back(pos);
                oldToNewIndex.push_back(newIdx);
            } else {
                // Duplicate in this file
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
            
            indices_.push_back(indices[0]);
            indices_.push_back(indices[1]);
            indices_.push_back(indices[2]);
            
            // Compute face normal
            glm::vec3 v0 = vertices_[indices[0]];
            glm::vec3 v1 = vertices_[indices[1]];
            glm::vec3 v2 = vertices_[indices[2]];
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            
            faceNormals_.push_back(normal);
        }
    }
    
    size_t trianglesBefore = faceNormals_.size();
    size_t trianglesAfter = indices_.size() / 3;
    size_t trianglesAdded = trianglesAfter - trianglesBefore;
    
    std::cout << "Appended OBJ file:\n"
              << "  Vertices including duplicates: " << rawVertexCount << "\n"
              << "  Vertices excluding duplicates (in this file): " << verticesMap.size() << "\n"
              << "  Triangles added: " << trianglesAdded << std::endl;
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
        bool isThereNonManifoldEdge = false;
        for (int i = 0; i < 3; i++) {
            int v0 = indices_[faceIdx * 3 + i];
            int v1 = indices_[faceIdx * 3 + (i + 1) % 3];
            if (edgeToFaces_.at(makeEdge(v0, v1)).size() == 1) {
                numBoundaryEdges++;
            }
            if (edgeToFaces_.at(makeEdge(v0, v1)).size() > 2) {
                isThereNonManifoldEdge = true;
            }
        }
        // boundary edge 1
        if (numBoundaryEdges > 0 && numBoundaryEdges < 3 && isThereNonManifoldEdge) {
            boundaryFaces_[numBoundaryEdges - 1].push_back(faceIdx);
        }
        if (numBoundaryEdges == 3) {
            boundaryFaces_[numBoundaryEdges - 1].push_back(faceIdx);
        }
    }
    
    std::cout << "Boundary faces:\n"
              << "  1 boundary edge: " << boundaryFaces_[0].size() << "\n"
              << "  2 boundary edges: " << boundaryFaces_[1].size() << "\n"
              << "  3 boundary edges: " << boundaryFaces_[2].size() << std::endl;
}

void Mesh::analyzeComponents() const {
    int numTriangles = static_cast<int>(indices_.size() / 3);
    std::vector<bool> visited(numTriangles, false);
    std::vector<std::vector<int>> components;
    
    // Helper function to get neighbor faces through shared edges
    auto getNeighborFaces = [this](int faceIdx) -> std::vector<int> {
        std::vector<int> neighbors;
        std::unordered_set<int> neighborSet;
        
        // Get 3 edges of this face
        int v0 = indices_[faceIdx * 3 + 0];
        int v1 = indices_[faceIdx * 3 + 1];
        int v2 = indices_[faceIdx * 3 + 2];
        
        Edge edges[3] = {
            makeEdge(v0, v1),
            makeEdge(v1, v2),
            makeEdge(v2, v0)
        };
        
        // For each edge, find all faces that share it
        for (const auto& edge : edges) {
            auto it = edgeToFaces_.find(edge);
            if (it != edgeToFaces_.end()) {
                for (int neighborFaceIdx : it->second) {
                    if (neighborFaceIdx != faceIdx) {
                        neighborSet.insert(neighborFaceIdx);
                    }
                }
            }
        }
        
        neighbors.assign(neighborSet.begin(), neighborSet.end());
        return neighbors;
    };
    
    // BFS to find connected components
    for (int startFace = 0; startFace < numTriangles; ++startFace) {
        if (visited[startFace]) {
            continue;
        }
        
        // Start a new component
        std::vector<int> component;
        std::vector<int> queue;
        queue.push_back(startFace);
        visited[startFace] = true;
        
        while (!queue.empty()) {
            int currentFace = queue.back();
            queue.pop_back();
            component.push_back(currentFace);
            
            // Get neighbors
            std::vector<int> neighbors = getNeighborFaces(currentFace);
            for (int neighbor : neighbors) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    queue.push_back(neighbor);
                }
            }
        }
        
        components.push_back(component);
    }
    
    // Sort components by size (largest first)
    std::sort(components.begin(), components.end(), 
              [](const std::vector<int>& a, const std::vector<int>& b) {
                  return a.size() > b.size();
              });
    
    // Store all components
    components_ = components;
    
    // Output results
    std::cout << "Connected components:\n"
              << "  Total components: " << components.size() << std::endl;
    
    if (!components.empty()) {
        std::cout << "  Component sizes (largest first):\n";
        int displayCount = std::min(10, static_cast<int>(components.size()));
        for (int i = 0; i < displayCount; ++i) {
            std::cout << "    Component " << (i + 1) << ": " 
                      << components[i].size() << " faces";
            if (i == 0) {
                std::cout << " (largest)";
            }
            if (i == 1) {
                std::cout << " (2nd largest)";
            }
            std::cout << std::endl;
        }
        if (components.size() > displayCount) {
            std::cout << "    ... (" << (components.size() - displayCount) 
                      << " more components)" << std::endl;
        }
        if (components.size() >= 1) {
            std::cout << "  Highlighting largest component (" 
                      << components[0].size() << " faces) in green" << std::endl;
        }
        if (components.size() >= 2) {
            std::cout << "  Highlighting 2nd largest component (" 
                      << components[1].size() << " faces) in blue" << std::endl;
        }
    }
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

