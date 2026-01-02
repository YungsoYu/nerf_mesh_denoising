#include "mesh_renderer.h"
#include <unordered_set>
#include <algorithm>

MeshRenderer::~MeshRenderer() {
    cleanup();
}

MeshRenderer::MeshRenderer(MeshRenderer&& other) noexcept
    : VAO_(other.VAO_)
    , VBO_(other.VBO_)
    , vertexCount_(other.vertexCount_)
{
    other.VAO_ = 0;
    other.VBO_ = 0;
    other.vertexCount_ = 0;
}

MeshRenderer& MeshRenderer::operator=(MeshRenderer&& other) noexcept {
    if (this != &other) {
        cleanup();
        VAO_ = other.VAO_;
        VBO_ = other.VBO_;
        vertexCount_ = other.vertexCount_;
        
        other.VAO_ = 0;
        other.VBO_ = 0;
        other.vertexCount_ = 0;
    }
    return *this;
}

void MeshRenderer::cleanup() {
    if (VAO_ != 0) {
        glDeleteVertexArrays(1, &VAO_);
        VAO_ = 0;
    }
    if (VBO_ != 0) {
        glDeleteBuffers(1, &VBO_);
        VBO_ = 0;
    }
    vertexCount_ = 0;
}

void MeshRenderer::upload(const Mesh& mesh, const bool highlightSelection[3]) {
    std::vector<float> glVertices = buildGLVertices(mesh, highlightSelection);
    vertexCount_ = static_cast<int>(glVertices.size() / 7);
    
    // Create VAO/VBO if needed
    if (VAO_ == 0) glGenVertexArrays(1, &VAO_);
    if (VBO_ == 0) glGenBuffers(1, &VBO_);
    
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 glVertices.size() * sizeof(float),
                 glVertices.data(),
                 GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // FaceColor attribute (location 2)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void MeshRenderer::draw() const {
    if (VAO_ == 0 || vertexCount_ == 0) {
        return;
    }
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}

std::vector<float> MeshRenderer::buildGLVertices(const Mesh& mesh, const bool highlightSelection[3]) const {
    // Build set of boundary faces (union of all selected types)
    std::unordered_set<int> boundaryFaces;
    for (int i = 0; i < 3; ++i) {
        if (highlightSelection[i]) {
            const auto& faces = mesh.boundaryFaces(i);
            boundaryFaces.insert(faces.begin(), faces.end());
        }
    }
    
    // Get components (sorted by size, largest first)
    const auto& components = mesh.components();
    
    // Build set of faces to render (only largest 2 components)
    std::unordered_set<int> facesToRender;
    size_t numTriangles = mesh.faceCount();
    for (size_t t = 0; t < numTriangles; ++t) {
        facesToRender.insert(t);  // 모든 face 추가
    }
    // if (components.size() >= 1) {
    //     facesToRender.insert(components[0].begin(), components[0].end());
    // }
    // if (components.size() >= 2) {
    //     facesToRender.insert(components[1].begin(), components[1].end());
    // }
    // facesToRender.insert(mesh.overlappingFaces_.begin(), mesh.overlappingFaces_.end());

    // Build sets of faces for each component (for fast lookup in coloring)
    std::unordered_set<int> greenFaces;  // Largest component
    std::unordered_set<int> blueFaces;    // 2nd largest component
    // if (components.size() >= 1) {
    //     greenFaces.insert(components[0].begin(), components[0].end());
    // }
    if (components.size() >= 2) {
        blueFaces.insert(components[1].begin(), components[1].end());
    }
    
    // Build interleaved vertex data: position(3) + normal(3) + faceColor(1)
    // faceColor values: 0.0 = normal, 1.0 = boundary, 2.0 = green component, 3.0 = blue component
    std::vector<float> glVertices;
    
    glVertices.reserve(facesToRender.size() * 3 * 7);
    
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    const auto& faceNormals = mesh.faceNormals();
    
    for (size_t t = 0; t < numTriangles; ++t) {
        if (facesToRender.count(t) == 0) {
            continue; 
        }
        
        glm::vec3 normal = faceNormals[t];
        float faceColor;
        if (greenFaces.count(t)) {
            faceColor = FaceColor::GREEN_COMPONENT;
        } else if (blueFaces.count(t)) {
            faceColor = FaceColor::BLUE_COMPONENT;
        } else if (boundaryFaces.count(t)) {
            faceColor = FaceColor::BOUNDARY;
        } else {
            faceColor = FaceColor::NORMAL;
        }
        
        for (int v = 0; v < 3; ++v) {
            int idx = indices[t * 3 + v];
            glm::vec3 pos = vertices[idx];
            
            glVertices.push_back(pos.x);
            glVertices.push_back(pos.y);
            glVertices.push_back(pos.z);
            glVertices.push_back(normal.x);
            glVertices.push_back(normal.y);
            glVertices.push_back(normal.z);
            glVertices.push_back(faceColor);
        }
    }
    
    return glVertices;
}
