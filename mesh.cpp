#include "mesh.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>

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
    
    return mesh;
}

void prepareMeshForGL(Mesh& mesh)
{
    // Build interleaved vertex data: position(3) + normal(3)
    mesh.glVertices.clear();
    
    size_t numTriangles = mesh.indices.size() / 3;
    mesh.glVertices.reserve(numTriangles * 3 * 6);
    
    for (size_t t = 0; t < numTriangles; ++t) {
        glm::vec3 normal = mesh.faceNormals[t];
        
        for (int v = 0; v < 3; ++v) {
            int idx = mesh.indices[t * 3 + v];
            glm::vec3 pos = mesh.vertices[idx];
            
            mesh.glVertices.push_back(pos.x);
            mesh.glVertices.push_back(pos.y);
            mesh.glVertices.push_back(pos.z);
            mesh.glVertices.push_back(normal.x);
            mesh.glVertices.push_back(normal.y);
            mesh.glVertices.push_back(normal.z);
        }
    }
    
    mesh.vertexCount = static_cast<int>(mesh.glVertices.size() / 6);
    
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}
