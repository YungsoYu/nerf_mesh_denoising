#include "mesh_renderer.h"
#include <unordered_set>

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

void MeshRenderer::upload(const Mesh& mesh, int highlightSelection) {
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
    
    // IsBoundary attribute (location 2)
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

std::vector<float> MeshRenderer::buildGLVertices(const Mesh& mesh, int highlightSelection) const {
    // Build set of faces to highlight
    std::unordered_set<int> highlightFaces;
    if (highlightSelection >= 0 && highlightSelection <= 2) {
        const auto& faces = mesh.boundaryFaces(highlightSelection);
        highlightFaces.insert(faces.begin(), faces.end());
    }
    
    // Build interleaved vertex data: position(3) + normal(3) + isBoundary(1)
    std::vector<float> glVertices;
    
    size_t numTriangles = mesh.faceCount();
    glVertices.reserve(numTriangles * 3 * 7);
    
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    const auto& faceNormals = mesh.faceNormals();
    
    for (size_t t = 0; t < numTriangles; ++t) {
        glm::vec3 normal = faceNormals[t];
        float isBoundary = highlightFaces.count(t) ? 1.0f : 0.0f;
        
        for (int v = 0; v < 3; ++v) {
            int idx = indices[t * 3 + v];
            glm::vec3 pos = vertices[idx];
            
            glVertices.push_back(pos.x);
            glVertices.push_back(pos.y);
            glVertices.push_back(pos.z);
            glVertices.push_back(normal.x);
            glVertices.push_back(normal.y);
            glVertices.push_back(normal.z);
            glVertices.push_back(isBoundary);
        }
    }
    
    return glVertices;
}
