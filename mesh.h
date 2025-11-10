#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <vector>
#include <string>

struct Mesh {
    std::vector<float> vertices;
    unsigned int VAO, VBO;
    int vertexCount;
};

Mesh loadOBJ(const std::string& path);

#endif

