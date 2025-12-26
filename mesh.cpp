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

Mesh loadOBJ(const std::string& path)
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
                int newIdx = static_cast<int>(mesh.vertices.size());
                verticesMap[pos] = newIdx;
                mesh.vertices.push_back(pos);
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
            
            mesh.indices.push_back(indices[0]);
            mesh.indices.push_back(indices[1]);
            mesh.indices.push_back(indices[2]);
            
            // Compute face normal
            glm::vec3 v0 = mesh.vertices[indices[0]];
            glm::vec3 v1 = mesh.vertices[indices[1]];
            glm::vec3 v2 = mesh.vertices[indices[2]];
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            
            mesh.faceNormals.push_back(normal);
        }
    }
    
    std::cout << "loaded OBJ file and removed duplicates: \n"
     << "vertices including duplicates: " << rawVertexCount
     << "\nvertices excluding duplicates: " << mesh.vertices.size() 
     << "\ntriangles " << mesh.indices.size() / 3 << std::endl;

    mesh.edgeToFaces = buildEdgeFaceAdjacency(mesh);
    analyzeMesh(mesh.edgeToFaces);
    findBoundaryFaces(mesh);

    return mesh;
}

std::map<Edge, std::vector<int>> buildEdgeFaceAdjacency(const Mesh& mesh) {
    std::map<Edge, std::vector<int>> edgeToFaces;
    
    size_t numTriangles = mesh.indices.size() / 3;
    for (size_t faceIdx = 0; faceIdx < numTriangles; ++faceIdx) {
        int v0 = mesh.indices[faceIdx * 3 + 0];
        int v1 = mesh.indices[faceIdx * 3 + 1];
        int v2 = mesh.indices[faceIdx * 3 + 2];
        
        edgeToFaces[makeEdge(v0, v1)].push_back(faceIdx);
        edgeToFaces[makeEdge(v1, v2)].push_back(faceIdx);
        edgeToFaces[makeEdge(v2, v0)].push_back(faceIdx);
    }
    return edgeToFaces;
}

void analyzeMesh(const std::map<Edge, std::vector<int>>& edgeToFaces) {
    int boundaryEdges = 0;
    int manifoldEdges = 0;
    int nonManifoldEdges_3 = 0;
    int nonManifoldEdges_4 = 0;
    int nonManifoldEdges = 0;

    for (const auto& [edge, faceList] : edgeToFaces) {
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

    std::cout << "Total edges: " << edgeToFaces.size() << "\n";
    std::cout << "Boundary edges (1 face): " << boundaryEdges << "\n";
    std::cout << "Manifold edges (2 faces): " << manifoldEdges << "\n";
    std::cout << "Non-Manifold edges (3 faces): " << nonManifoldEdges_3 << "\n";
    std::cout << "Non-Manifold edges (4 faces): " << nonManifoldEdges_4 << "\n";
    std::cout << "Non-manifold edges (5+ faces): " << nonManifoldEdges << "\n";
}

void findBoundaryFaces(Mesh& mesh) { 
    int numTriangles = mesh.indices.size() / 3;
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {

        int numBoundaryEdges = 0;
        for (int i = 0; i < 3; i++) {  // Check all 3 edges
            int v0 = mesh.indices[faceIdx * 3 + i];
            int v1 = mesh.indices[faceIdx * 3 + (i + 1) % 3];  // Wrap around
            if (mesh.edgeToFaces[makeEdge(v0, v1)].size() == 1) {
                numBoundaryEdges++;
            }
        }
        if (numBoundaryEdges == 1) {
            mesh.boundaryFaces_1.push_back(faceIdx);
        } else if (numBoundaryEdges == 2) {
            mesh.boundaryFaces_2.push_back(faceIdx);
        } else if (numBoundaryEdges == 3) {
            mesh.boundaryFaces_3.push_back(faceIdx);
        }
    }
    std::cout << "Boundary faces (1 edge): " << mesh.boundaryFaces_1.size() << "\n";
    std::cout << "Boundary faces (2 edge): " << mesh.boundaryFaces_2.size() << "\n";
    std::cout << "Boundary faces (3 edge): " << mesh.boundaryFaces_3.size() << "\n";
}

void removeBoundaryFaces(Mesh& mesh, int boundarySelection) {
    // Get the appropriate boundary faces based on selection
    const std::vector<int>* facesToRemoveVec = nullptr;
    if (boundarySelection == 0) {
        facesToRemoveVec = &mesh.boundaryFaces_1;
    } else if (boundarySelection == 1) {
        facesToRemoveVec = &mesh.boundaryFaces_2;
    } else {
        facesToRemoveVec = &mesh.boundaryFaces_3;
    }
    
    if (facesToRemoveVec->empty()) {
        return;
    }
    
    // Build set for fast lookup
    std::unordered_set<int> facesToRemove(facesToRemoveVec->begin(), facesToRemoveVec->end());
    
    // Filter faces - skip those in the removal set
    std::vector<int> newIndices;
    std::vector<glm::vec3> newNormals;
    
    int numTriangles = mesh.indices.size() / 3;
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {
        if (facesToRemove.count(faceIdx) == 0) {
            // Keep this face
            newIndices.push_back(mesh.indices[faceIdx * 3 + 0]);
            newIndices.push_back(mesh.indices[faceIdx * 3 + 1]);
            newIndices.push_back(mesh.indices[faceIdx * 3 + 2]);
            newNormals.push_back(mesh.faceNormals[faceIdx]);
        }
    }
    
    // Removed (show selected boundary)
    std::cout << "Removed " << facesToRemove.size() << " boundary faces\n";
    
    mesh.indices = std::move(newIndices);
    mesh.faceNormals = std::move(newNormals);
    
    // Rebuild adjacency and boundary info after modification
    mesh.boundaryFaces_1.clear();
    mesh.boundaryFaces_2.clear();
    mesh.boundaryFaces_3.clear();
    mesh.edgeToFaces = buildEdgeFaceAdjacency(mesh);
    analyzeMesh(mesh.edgeToFaces);
    findBoundaryFaces(mesh);
}

void prepareMeshForGL(Mesh& mesh, int highlightSelection)
{
    // Build set of faces to highlight
    std::unordered_set<int> highlightFaces;
    if (highlightSelection == 0) {
        highlightFaces.insert(mesh.boundaryFaces_1.begin(), mesh.boundaryFaces_1.end());
    } else if (highlightSelection == 1) {
        highlightFaces.insert(mesh.boundaryFaces_2.begin(), mesh.boundaryFaces_2.end());
    } else if (highlightSelection == 2) {
        highlightFaces.insert(mesh.boundaryFaces_3.begin(), mesh.boundaryFaces_3.end());
    }
    
    // Build interleaved vertex data: position(3) + normal(3) + isBoundary(1)
    mesh.glVertices.clear();
    
    size_t numTriangles = mesh.indices.size() / 3;
    mesh.glVertices.reserve(numTriangles * 3 * 7);
    
    for (size_t t = 0; t < numTriangles; ++t) {
        glm::vec3 normal = mesh.faceNormals[t];
        float isBoundary = highlightFaces.count(t) ? 1.0f : 0.0f;
        
        for (int v = 0; v < 3; ++v) {
            int idx = mesh.indices[t * 3 + v];
            glm::vec3 pos = mesh.vertices[idx];
            
            mesh.glVertices.push_back(pos.x);
            mesh.glVertices.push_back(pos.y);
            mesh.glVertices.push_back(pos.z);
            mesh.glVertices.push_back(normal.x);
            mesh.glVertices.push_back(normal.y);
            mesh.glVertices.push_back(normal.z);
            mesh.glVertices.push_back(isBoundary);
        }
    }
    
    mesh.vertexCount = static_cast<int>(mesh.glVertices.size() / 7);
    
    // Upload to GPU
    if (mesh.VAO == 0) glGenVertexArrays(1, &mesh.VAO);
    if (mesh.VBO == 0) glGenBuffers(1, &mesh.VBO);
    
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 mesh.glVertices.size() * sizeof(float),
                 mesh.glVertices.data(),
                 GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // IsBoundary attribute (location 2)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}
