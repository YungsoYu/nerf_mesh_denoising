#include "../include/mesh.h"
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <dirent.h>

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

void Mesh::loadFromOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << path << std::endl;
        return;
    }
    
    // Build global verticesMap from existing vertices_ (across all previously loaded files)
    std::unordered_map<glm::vec3, int, Vec3Hash, Vec3Equal> verticesMap;
    for (size_t i = 0; i < vertices_.size(); ++i) {
        verticesMap[vertices_[i]] = static_cast<int>(i);
    }
    
    // index mapping from old (duplicated vertices in original file) to new (unique vertices in global verticesMap)
    std::vector<int> oldToNewIndex;
    
    size_t trianglesBefore = indices_.size() / 3;
    int rawVertexCount = 0;
    int newVerticesAdded = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            // Parse vertex position
            std::istringstream iss(line.substr(2));
            float x, y, z;
            iss >> x >> y >> z;
            glm::vec3 pos(x, y, z);
            
            // Check if this position already exists in global map (across all files)
            auto it = verticesMap.find(pos);
            if (it == verticesMap.end()) {
                // New unique vertex across all files - add to mesh
                int newIdx = static_cast<int>(vertices_.size());
                verticesMap[pos] = newIdx;
                vertices_.push_back(pos);
                oldToNewIndex.push_back(newIdx);
                newVerticesAdded++;
            } else {
                // Duplicate - reuse existing vertex index from global map
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
            faceColors_.push_back(0.0f);
        }
    }

    size_t trianglesAfter = indices_.size() / 3;
    size_t trianglesAdded = trianglesAfter - trianglesBefore;
    
    std::cout << "Appended OBJ file (global deduplication):\n"
              << "  Vertices including duplicates: " << rawVertexCount << "\n"
              << "  New vertices added (excluding all duplicates): " << newVerticesAdded << "\n"
              << "  Total unique vertices (across all files): " << verticesMap.size() << "\n"
              << "  Triangles added: " << trianglesAdded << std::endl;
}

Mesh Mesh::createMesh(const std::string& meshDir) {
    Mesh mesh;
    DIR* dir = opendir(meshDir.c_str());
    if (!dir) {
        std::cerr << "Failed to open directory: " << meshDir << std::endl;
        return mesh;
    }
    
    std::vector<std::string> objPaths;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".obj") {
            objPaths.push_back(meshDir + filename);
        }
    }
    closedir(dir);

    if (objPaths.empty()) {
        std::cerr << "No OBJ files found in directory: " << meshDir << std::endl;
        return mesh;
    }

    // Load all OBJ files and append them to the mesh
    for (const auto& objPath : objPaths) {
        mesh.loadFromOBJ(objPath);
    }

    
    return mesh;
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
    numEdgesWith3Faces_ = 0;
    numEdgesWith4Faces_ = 0;
    int nonManifoldEdges = 0;

    for (const auto& [edge, faceList] : edgeToFaces_) {
        size_t count = faceList.size();
        
        if (count == 1) {
            boundaryEdges++;
        } else if (count == 2) {
            manifoldEdges++;
        } else if (count == 3) {
            numEdgesWith3Faces_++;
        } else if (count == 4) {
            numEdgesWith4Faces_++;
        } else {
            nonManifoldEdges++;
        }
    }


    std::cout << "Edge analysis:\n"
              << "  Total edges: " << edgeToFaces_.size() << "\n"
              << "  Boundary edges (1 face): " << boundaryEdges << "\n"
              << "  Manifold edges (2 faces): " << manifoldEdges << "\n"
              << "  Non-manifold edges (3 faces): " << numEdgesWith3Faces_ << "\n"
              << "  Non-manifold edges (4 faces): " << numEdgesWith4Faces_ << "\n"
              << "  Non-manifold edges (5+ faces): " << nonManifoldEdges << std::endl;
}

void Mesh::buildFaceStatus() {
    std::cout << "◦ Building Face Statues" << std::endl;
    int numTriangles = indices_.size() / 3;
    
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {
        int boundaryCount = 0;
        int manifoldCount = 0;
        int nonManifoldCount = 0;
        
        for (int i = 0; i < 3; i++) {
            int v0 = indices_[faceIdx * 3 + i];
            int v1 = indices_[faceIdx * 3 + (i + 1) % 3];
            Edge edge = makeEdge(v0, v1);
            
            auto it = edgeToFaces_.find(edge);
            if (it == edgeToFaces_.end()) {
                std::cout << "!! Warning: edge not found in edgeToFaces_ map" << std::endl;
                continue;
            }
            
            int edgeFaceCount = it->second.size();
            
            if (edgeFaceCount == 1) {
                boundaryCount++;
            } else if (edgeFaceCount == 2) {
                manifoldCount++;
            } else {
                nonManifoldCount++;
            }
        }
        faceBoundaryCount_[faceIdx] = boundaryCount;
        faceManifoldCount_[faceIdx] = manifoldCount;
        faceNonManifoldCount_[faceIdx] = nonManifoldCount;
    }
    std::cout << "• Completed Face Statues" << std::endl;

}

void Mesh::findBoundaryFaces() {
    int numTriangles = indices_.size() / 3;
    
    // Lambda function to get neighbor faces through manifold edges
    auto getNeighborFaces = [this](int faceIdx) -> std::vector<int> {
        std::vector<int> neighborFaces;
        
        // manifold edge 찾기
        for (int i = 0; i < 3; i++) {
            int v0 = indices_[faceIdx * 3 + i];
            int v1 = indices_[faceIdx * 3 + (i + 1) % 3];
            Edge edge = makeEdge(v0, v1);
            auto it = edgeToFaces_.find(edge);
            if (it == edgeToFaces_.end()) {
                continue;  // Edge가 맵에 없는 경우
            }
            int edgeFaceCount = it->second.size();
            if (edgeFaceCount == 2) { // manifold edge
                // 이 edge를 공유하는 모든 face들 가져오기 (1개만 있어야함)
                const auto& facesOnEdge = it->second;
                for (int neighborFaceIdx : facesOnEdge) {
                    if (neighborFaceIdx != faceIdx) {  // 현재 face 제외
                        neighborFaces.push_back(neighborFaceIdx);
                    }
                }
            }
        }
        
        return neighborFaces;
    };
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {

        if (faceBoundaryCount_[faceIdx] == 1) { // Face with 1 boundary edge
            // 기존 조건: boundary edge == 1 && non-mainfold edge == 2 -> remove
            // 추가 조건: 나머지 두 edge에 연결된 neighbor face 확인
            bool shouldAdd = false;

            if (faceManifoldCount_[faceIdx] == 0) {// if (nonManifoldEdge == 2) {
                shouldAdd = true;
            } else if (faceManifoldCount_[faceIdx] == 1) { 
                std::vector<int> neighborFaces = getNeighborFaces(faceIdx);
                
                // 이웃 face들의 edge 확인
                int boundaryInNeighbors = 0;
                int nonManifoldInNeighbors = 0;
                
                for (int neighborIdx : neighborFaces) {
                    if (neighborIdx >= 0 && neighborIdx < static_cast<int>(faceBoundaryCount_.size()) && 
                        faceBoundaryCount_[neighborIdx] > 0) {
                        boundaryInNeighbors++;
                    }
                }
                
                if (boundaryInNeighbors > 0) {
                    shouldAdd = true;
                }
            } else if (faceManifoldCount_[faceIdx] == 2) {

            }
            
            if (shouldAdd) {
                faceColors_[faceIdx] = 1;
            }

        } else if (faceBoundaryCount_[faceIdx] == 2) {
            bool shouldAdd = false;
            if (faceManifoldCount_[faceIdx] == 1) {
                shouldAdd = true;
            } else {
                std::vector<int> neighborFaces = getNeighborFaces(faceIdx);
                
                // 이웃 face들의 edge 확인
                int boundaryInNeighbors = 0;
                int nonManifoldInNeighbors = 0;
                
                for (int neighborIdx : neighborFaces) {
                    if (neighborIdx >= 0 && neighborIdx < static_cast<int>(faceBoundaryCount_.size()) && 
                        faceBoundaryCount_[neighborIdx] > 0) {
                        boundaryInNeighbors++;
                    }
                }
                
                if (boundaryInNeighbors > 0) {
                    shouldAdd = true;
                }
            }
            
            if (shouldAdd) {
                faceColors_[faceIdx] = 2;
            }
            // todo: adjacent face가 boundary edge를 두개 가진 경우 삭제
        } 
    }
    
    // todo: print

}

void Mesh::buildValence() {
    vertexValences_.clear();
    vertexValences_.resize(vertices_.size(), 0);
    
    std::vector<std::unordered_set<int>> neighbors(vertices_.size());
    
    size_t numTriangles = indices_.size() / 3;
    for (size_t faceIdx = 0; faceIdx < numTriangles; ++faceIdx) {
        size_t idxBase = faceIdx * 3;
        if (idxBase + 2 >= indices_.size()) {
            continue;  // 인덱스 범위 체크
        }
        
        int v0 = indices_[idxBase + 0];
        int v1 = indices_[idxBase + 1];
        int v2 = indices_[idxBase + 2];
        
        // Vertex 인덱스 유효성 체크
        if (v0 < 0 || v0 >= static_cast<int>(vertices_.size()) ||
            v1 < 0 || v1 >= static_cast<int>(vertices_.size()) ||
            v2 < 0 || v2 >= static_cast<int>(vertices_.size())) {
            continue;  // 잘못된 인덱스 스킵
        }
        
        // Add neighbors for each vertex in this triangle
        neighbors[v0].insert(v1);
        neighbors[v0].insert(v2);
        neighbors[v1].insert(v0);
        neighbors[v1].insert(v2);
        neighbors[v2].insert(v0);
        neighbors[v2].insert(v1);
    }
    
    // Calculate valence for each vertex (number of unique neighbors)
    for (size_t i = 0; i < vertices_.size(); ++i) {
        vertexValences_[i] = static_cast<int>(neighbors[i].size());
    }
    
    if (!vertexValences_.empty()) {
        int minValence = *std::min_element(vertexValences_.begin(), vertexValences_.end());
        int maxValence = *std::max_element(vertexValences_.begin(), vertexValences_.end());
        double avgValence = 0.0;
        for (int v : vertexValences_) {
            avgValence += v;
        }
        avgValence /= vertexValences_.size();
        
        std::cout << "Vertex valence computed:\n"
                  << "  Total vertices: " << vertexValences_.size() << "\n"
                  << "  Min valence: " << minValence << "\n"
                  << "  Max valence: " << maxValence << "\n"
                  << "  Average valence: " << avgValence << std::endl;
    }
}

void Mesh::findNonManifoldFacesToRemove() { // 3-manifold (valance)

    // 빠른 조회를 위한 set
    std::unordered_set<int> facesToRemoveSet;

    int skippedCount = 0;
    int manifoldCount = 0;

    for (const auto& [edge, faceList] : edgeToFaces_) {
        // Manifold edge만 처리 (정확히 2개의 face를 가진 edge)
        if (faceList.size() < 3) {
            continue;
        }
        
        // faceList 중에 이미 facesToRemove에 있는 face가 있으면 다음 edge로 넘어감
        // bool hasRemovedFace = false;
        // for (int fi : faceList) {
        //     if (facesToRemoveSet.count(fi) > 0) {
        //         hasRemovedFace = true;
        //         break;
        //     }
        // }
        // if (hasRemovedFace) {
        //     continue;
        // }

        // 1. edge의 두 vertex 추출
        int edgeV0 = edge.first;
        int edgeV1 = edge.second;

        // 2. 각 face에 대해 edge에 포함되지 않은 vertex와 그 valence 저장
        struct FaceValenceInfo {
            int faceIdx;
            int oppositeVertex;
            int valence;
        };

        std::vector<FaceValenceInfo> faceInfos;

        for (int faceIdx : faceList) { // 각 페이스에 대해 opposite vertex와 valence 저장
            // face의 세 vertex 가져오기
            int v0 = indices_[faceIdx * 3 + 0];
            int v1 = indices_[faceIdx * 3 + 1];
            int v2 = indices_[faceIdx * 3 + 2];
            
            // edge에 포함되지 않은 vertex 찾기
            int oppositeVertex = -1;
            if (v0 != edgeV0 && v0 != edgeV1) {
                oppositeVertex = v0;
            } else if (v1 != edgeV0 && v1 != edgeV1) {
                oppositeVertex = v1;
            } else {
                oppositeVertex = v2;  // v2가 edge에 포함되지 않은 vertex
            }
            
            // 해당 vertex의 valence 가져오기
            int valence = vertexValences_[oppositeVertex];
            
            faceInfos.push_back({faceIdx, oppositeVertex, valence});
        }

        // faceInfos가 비어있으면 스킵
        if (faceInfos.empty()) {
            continue;
        }

        // 3. 가장 작은 valence를 갖는 face 찾기
        auto minIt = std::min_element(faceInfos.begin(), faceInfos.end(),
            [](const FaceValenceInfo& a, const FaceValenceInfo& b) {
                return a.valence < b.valence;
            });

        int minValence = minIt->valence;
        
        // 4. 같은 최소 valence를 가진 face가 여러 개인지 확인
        int minValenceCount = 0;
        for (const auto& info : faceInfos) {
            if (info.valence == minValence) {
                minValenceCount++;
            }
        }
        // 같은 valence를 가진 face가 여러 개면 스킵
        // if (minValenceCount > 1) {
        //     skippedCount++;
        //     continue;
        // }

        int faceToRemove = minIt->faceIdx;

        // 5. 삭제하려는 face의 다른 두 edge에 대해 adjacent face 개수 확인
        // face의 세 vertex 가져오기
        int v0 = indices_[faceToRemove * 3 + 0];
        int v1 = indices_[faceToRemove * 3 + 1];
        int v2 = indices_[faceToRemove * 3 + 2];
        
        // face의 세 edge
        Edge edge1 = makeEdge(v0, v1);
        Edge edge2 = makeEdge(v1, v2);
        Edge edge3 = makeEdge(v2, v0);
        
        // 현재 처리 중인 edge와 다른 두 edge 찾기
        Edge otherEdge1, otherEdge2;
        if (edge1 == edge) {
            // 현재 edge가 edge1이면, 나머지 두 edge는 edge2, edge3
            otherEdge1 = edge2;
            otherEdge2 = edge3;
        } else if (edge2 == edge) {
            // 현재 edge가 edge2이면, 나머지 두 edge는 edge1, edge3
            otherEdge1 = edge1;
            otherEdge2 = edge3;
        } else {
            // 현재 edge가 edge3이면, 나머지 두 edge는 edge1, edge2
            otherEdge1 = edge1;
            otherEdge2 = edge2;
        }
        
        // 각 edge의 adjacent face 개수 확인
        auto it1 = edgeToFaces_.find(otherEdge1);
        auto it2 = edgeToFaces_.find(otherEdge2);
        
        int adjCount1 = (it1 != edgeToFaces_.end()) ? static_cast<int>(it1->second.size()) : 0;
        int adjCount2 = (it2 != edgeToFaces_.end()) ? static_cast<int>(it2->second.size()) : 0;
        

        // if (adjCount1 < 3 && adjCount2 < 3) {
        //     manifoldCount++;
        //     continue;
        // }

        faceColors_[faceToRemove] = 3;
        facesToRemoveSet.insert(faceToRemove);

    }

    std::cout << "Skipped count (no min): " << skippedCount << std::endl;

    std::cout << "Skipped count (mainfold): " << manifoldCount << std::endl;
    // std::cout << "Removed faces: " << nonManifoldToRemove.size() << std::endl;
}


std::vector<std::vector<int>> Mesh::getComponents() {
    std::cout << "◦ Progressing: component analysis ..." << std::endl;

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
    
    // Output results
    std::cout << "• Completed: component analysis\n"
              << "  - total components: " << components.size() << std::endl;

    
    for (int i = 0; i < components.size(); ++i) {
        std::cout << "    Component " << (i + 1) << ": " 
                    << components[i].size() << " faces";

        std::cout << std::endl;
        if (i > 7) {
            break;
        }
    }

    return components;
}

void Mesh::removeFaces(const std::vector<int>& facesToRemoveVec) {
    if (facesToRemoveVec.empty()) {
        return;
    }
    
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
    
    std::cout << "Removed " << facesToRemove.size() << " faces" << std::endl;
    
    indices_ = std::move(newIndices);
    faceNormals_ = std::move(newNormals);
    

    // faceColors_도 함께 업데이트 (삭제된 face 제거)
    std::vector<float> newFaceColors;
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {
        if (facesToRemove.count(faceIdx) == 0) {
            newFaceColors.push_back(faceColors_[faceIdx]);
        }
    }
    faceColors_ = std::move(newFaceColors);

    buildEdgeFaceAdjacency();
    std::vector<std::vector<int>> components = getComponents(); 
    
    if (components.size() > 1) {
        removeRestComponents(components);
    } else {
        resetFaceData();
        analyzeMesh();
        buildFaceStatus();
        findBoundaryFaces();
        buildValence();
        findNonManifoldFacesToRemove();
    }
}

void Mesh::resetFaceData() {
    size_t numTriangles = faceCount();
    
    faceBoundaryCount_.clear();
    faceManifoldCount_.clear();
    faceNonManifoldCount_.clear();
    faceColors_.clear();

    faceBoundaryCount_.resize(numTriangles, 0);
    faceManifoldCount_.resize(numTriangles, 0);
    faceNonManifoldCount_.resize(numTriangles, 0);
    faceColors_.resize(numTriangles, 0.0f);
}

void Mesh::removeBoundaryFaces(int boundarySelection) {
    if (boundarySelection < 0 || boundarySelection > 2) {
        return;
    }
    
    // faceColors[i] == boundarySelection+1에 해당하는 face index들 수집
    std::vector<int> facesToRemove;
    for (size_t i = 0; i < faceCount(); ++i) {
        if (static_cast<int>(faceColors_[i]) == boundarySelection + 1) {
            facesToRemove.push_back(static_cast<int>(i));
        }
    }
    
    removeFaces(facesToRemove);
    std::cout << " (boundary selection: " << boundarySelection << ")" << std::endl;
}

void Mesh::removeNonManifoldFaces() {
    std::cout << "◦ Processing: removing non-manifold faces ..." << std::endl;

    std::vector<int> facesToRemove;
    for (size_t i = 0; i < faceCount(); ++i) {
        if (static_cast<int>(faceColors_[i]) == 3) {
            facesToRemove.push_back(static_cast<int>(i));
        }
    }
    
    if (numEdgesWith4Faces_ >= 0 && numEdgesWith3Faces_ > 10000) {
        
        if (facesToRemove.empty()) {
            std::cout << "• Failed: no non-manifold faces to remove" << std::endl;
        } else {
            removeFaces(facesToRemove);
            std::cout << "• Completed: non-manifold faces removed: " << facesToRemove.size() << " faces" << std::endl;
        }
    } else {
        std::cout << "• Failed: Too small non-manifold faces to remove " << std::endl;
    }
}

void Mesh::removeRestComponents(std::vector<std::vector<int>>& components) {
    // Keep only the largest component
    std::unordered_set<int> facesToRemove;

    if (components.size() > 1) {
        std::cout << "◦ Processing: removing other components except the largest component..." << std::endl;
        double component0Size = static_cast<double>(components[0].size());
        double component1Size = static_cast<double>(components[1].size());
        double percentage = (component1Size / component0Size) * 100.0;
    
        if (percentage < 10.0) {
            // Find all faces that should be removed (all components except the largest)
            for (int i = 1; i < components.size(); i++) {
                facesToRemove.insert(components[i].begin(), components[i].end());
            }
            
            // Convert unordered_set to vector for removeFaces
            std::vector<int> facesToRemoveVec(facesToRemove.begin(), facesToRemove.end());
            
            if (!facesToRemoveVec.empty()) {
                removeFaces(facesToRemoveVec);
                std::cout << "• Completed: removed " << facesToRemoveVec.size() << " faces from smaller components" << std::endl;
            }
        } else {
            std::cout << "✕ Warning: the second largest component is too large to be removed" << std::endl;
        }
    }
}
