#include "mesh.h"
#include <iostream>

#include <OpenMesh/Core/IO/MeshIO.hh>

Mesh loadOBJ(const std::string& path)
{
    Mesh mesh;

    if (!OpenMesh::IO::read_mesh(mesh.omesh, path)) {
        std::cerr << "OpenMesh: failed to read mesh: " << path << std::endl;
    }

    return mesh;
}

void prepareMeshForGL(Mesh& mesh, bool smoothNormals)
{
    // Request and compute normals
    if (smoothNormals) {
        mesh.omesh.request_vertex_normals();
        mesh.omesh.request_face_normals();
        mesh.omesh.update_normals();
    } else {
        mesh.omesh.request_face_normals();
        mesh.omesh.update_face_normals();
    }

    // Convert OpenMesh to interleaved triangle data: position(3) + normal(3)
    mesh.vertices.clear();
    mesh.vertices.reserve(static_cast<size_t>(mesh.omesh.n_faces()) * 3u * 6u);

    for (auto f_it = mesh.omesh.faces_begin(); f_it != mesh.omesh.faces_end(); ++f_it) {
        auto fv_it = mesh.omesh.cfv_iter(*f_it);

        OpenMesh::Vec3f p[3];
        OpenMesh::Vec3f n[3];
        int k = 0;
        for (; fv_it.is_valid() && k < 3; ++fv_it, ++k) {
            auto vh = *fv_it;
            p[k] = mesh.omesh.point(vh);
            if (smoothNormals && mesh.omesh.has_vertex_normals()) {
                n[k] = mesh.omesh.normal(vh);
            }
        }
        if (k != 3) continue;

        OpenMesh::Vec3f faceN = OpenMesh::cross(p[1] - p[0], p[2] - p[0]);
        if (faceN.norm() > 0.0f) faceN.normalize();

        for (int i = 0; i < 3; ++i) {
            OpenMesh::Vec3f nn = smoothNormals ? n[i] : faceN;
            mesh.vertices.push_back(p[i][0]); mesh.vertices.push_back(p[i][1]); mesh.vertices.push_back(p[i][2]);
            mesh.vertices.push_back(nn[0]);   mesh.vertices.push_back(nn[1]);   mesh.vertices.push_back(nn[2]);
        }
    }

    mesh.vertexCount = static_cast<int>(mesh.vertices.size() / 6);

    // Upload to GPU
    if (mesh.VAO == 0) glGenVertexArrays(1, &mesh.VAO);
    if (mesh.VBO == 0) glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 mesh.vertices.size() * sizeof(float),
                 mesh.vertices.data(),
                 GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}
