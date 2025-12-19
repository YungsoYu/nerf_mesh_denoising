#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <vector>
#include <string>

#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

using OpenMeshTriMesh = OpenMesh::TriMesh_ArrayKernelT<>;

struct Mesh {
    // OpenMesh data structure (connectivity + geometry). This is the "source of truth".
    OpenMeshTriMesh omesh;

    // OpenGL-ready interleaved data: position(3) + normal(3) per vertex, expanded to triangles.
    std::vector<float> vertices;
    unsigned int VAO = 0, VBO = 0;
    int vertexCount = 0;
};

// Load OBJ file into OpenMesh data structure only.
Mesh loadOBJ(const std::string& path);

// Convert OpenMesh to OpenGL VBO (interleaved triangles) and upload to GPU.
// Call this before drawing. smoothNormals=false gives flat shading (per-face normals).
void prepareMeshForGL(Mesh& mesh, bool smoothNormals = false);

#endif

