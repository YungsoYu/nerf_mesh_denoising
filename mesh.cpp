#include "mesh.h"
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

Mesh loadOBJ(const std::string& path) {
    Mesh mesh;
    std::vector<float> tempVertices;
    std::vector<unsigned int> indices;
    
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            std::istringstream iss(line.substr(2));
            float x, y, z;
            iss >> x >> y >> z;
            tempVertices.push_back(x);
            tempVertices.push_back(y);
            tempVertices.push_back(z);
        } else if (line.substr(0, 2) == "f ") {
            std::istringstream iss(line.substr(2));
            std::string token;
            while (iss >> token) {
                size_t pos = token.find('/');
                if (pos != std::string::npos) {
                    token = token.substr(0, pos);
                }
                indices.push_back(std::stoi(token) - 1);
            }
        }
    }
    
    // Build vertices with face normals (flat shading)
    // Each vertex now has: position (3) + normal (3) = 6 floats
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Get the three vertices of the triangle
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        glm::vec3 v0(tempVertices[i0 * 3], tempVertices[i0 * 3 + 1], tempVertices[i0 * 3 + 2]);
        glm::vec3 v1(tempVertices[i1 * 3], tempVertices[i1 * 3 + 1], tempVertices[i1 * 3 + 2]);
        glm::vec3 v2(tempVertices[i2 * 3], tempVertices[i2 * 3 + 1], tempVertices[i2 * 3 + 2]);

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        // Add all three vertices with the same face normal
        mesh.vertices.push_back(v0.x); mesh.vertices.push_back(v0.y); mesh.vertices.push_back(v0.z);
        mesh.vertices.push_back(normal.x); mesh.vertices.push_back(normal.y); mesh.vertices.push_back(normal.z);
        
        mesh.vertices.push_back(v1.x); mesh.vertices.push_back(v1.y); mesh.vertices.push_back(v1.z);
        mesh.vertices.push_back(normal.x); mesh.vertices.push_back(normal.y); mesh.vertices.push_back(normal.z);
        
        mesh.vertices.push_back(v2.x); mesh.vertices.push_back(v2.y); mesh.vertices.push_back(v2.z);
        mesh.vertices.push_back(normal.x); mesh.vertices.push_back(normal.y); mesh.vertices.push_back(normal.z);
    }
    
    mesh.vertexCount = mesh.vertices.size() / 6;  // 6 floats per vertex now
    
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    return mesh;
}

