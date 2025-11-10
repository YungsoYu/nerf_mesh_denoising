#include "mesh.h"
#include <fstream>
#include <sstream>

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
    
    for (unsigned int idx : indices) {
        mesh.vertices.push_back(tempVertices[idx * 3]);
        mesh.vertices.push_back(tempVertices[idx * 3 + 1]);
        mesh.vertices.push_back(tempVertices[idx * 3 + 2]);
    }
    
    mesh.vertexCount = mesh.vertices.size() / 3;
    
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    return mesh;
}

