#include "mesh.h"
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
    
    // Store current vertex count to offset indices
    int vertexOffset = static_cast<int>(vertices_.size());
    
    // vertex position - index pair (local to this file)
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
            
            // Check if this position already exists in local map
            auto it = verticesMap.find(pos);
            if (it == verticesMap.end()) {
                // New unique vertex - add to mesh
                int newIdx = static_cast<int>(vertices_.size());
                verticesMap[pos] = newIdx;
                vertices_.push_back(pos);
                oldToNewIndex.push_back(newIdx);
            } else {
                // Duplicate in this file
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
        }
    }
    
    size_t trianglesBefore = faceNormals_.size();
    size_t trianglesAfter = indices_.size() / 3;
    size_t trianglesAdded = trianglesAfter - trianglesBefore;
    
    std::cout << "Appended OBJ file:\n"
              << "  Vertices including duplicates: " << rawVertexCount << "\n"
              << "  Vertices excluding duplicates (in this file): " << verticesMap.size() << "\n"
              << "  Triangles added: " << trianglesAdded << std::endl;
}

void Mesh::loadFromOBJ2(const std::string& path) {
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
        }
    }
    
    size_t trianglesBefore = faceNormals_.size();
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
        mesh.loadFromOBJ2(objPath);
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
    nonManifoldWith3Faces_ = 0;
    nonManifoldWith4Faces_ = 0;
    int nonManifoldEdges = 0;

    for (const auto& [edge, faceList] : edgeToFaces_) {
        size_t count = faceList.size();
        
        if (count == 1) {
            boundaryEdges++;
        } else if (count == 2) {
            manifoldEdges++;
        } else if (count == 3) {
            nonManifoldWith3Faces_++;
        } else if (count == 4) {
            nonManifoldWith4Faces_++;
        } else {
            nonManifoldEdges++;
        }
    }


    std::cout << "Edge analysis:\n"
              << "  Total edges: " << edgeToFaces_.size() << "\n"
              << "  Boundary edges (1 face): " << boundaryEdges << "\n"
              << "  Manifold edges (2 faces): " << manifoldEdges << "\n"
              << "  Non-manifold edges (3 faces): " << nonManifoldWith3Faces_ << "\n"
              << "  Non-manifold edges (4 faces): " << nonManifoldWith4Faces_ << "\n"
              << "  Non-manifold edges (5+ faces): " << nonManifoldEdges << std::endl;
}

void Mesh::findBoundaryFaces() {
    for (auto& vec : boundaryFaces_) {
        vec.clear();
    }

    int oneBoundaryFace = 0;
    int caseCount = 0;
    
    int numTriangles = indices_.size() / 3;
    for (int faceIdx = 0; faceIdx < numTriangles; faceIdx++) {
        int numBoundaryEdges = 0;
        int nonManifoldEdge = 0;
        
        // 각 edge의 상태를 저장 (0: boundary, 1: manifold, 2: non-manifold)
        std::array<int, 3> edgeStates;
        
        for (int i = 0; i < 3; i++) {
            int v0 = indices_[faceIdx * 3 + i];
            int v1 = indices_[faceIdx * 3 + (i + 1) % 3];
            Edge edge = makeEdge(v0, v1);
            
            // edge가 존재하는지 확인
            auto it = edgeToFaces_.find(edge);
            if (it == edgeToFaces_.end()) {
                continue;  // edge가 없으면 스킵
            }
            
            int edgeFaceCount = it->second.size();
            
            if (edgeFaceCount == 1) {
                numBoundaryEdges++;
                edgeStates[i] = 0;  // boundary
            } else if (edgeFaceCount > 2) {
                nonManifoldEdge++;
                edgeStates[i] = 2;  // non-manifold
            } else {
                edgeStates[i] = 1;  // manifold
            }
        }

        // boundary edge 1
        if (numBoundaryEdges == 1) {
            // 기존 조건: nonManifoldEdge == 2
            // 추가 조건: 나머지 두 edge를 개별적으로 확인
            bool shouldAdd = false;
            oneBoundaryFace++;
            if (nonManifoldEdge == 2) {
                shouldAdd = true;
            } else if (nonManifoldEdge == 1) { // 한쪽은 non manifold, 다른 한쪽은 이웃 페이스가 boundary face, 한쪽은 현재 non mannifold
                // 이웃 face 두 개의 index 가져오기
                std::unordered_set<int> neighborFaces;
                
                // manifold edge 1개 찾기
                for (int i = 0; i < 3; i++) {
                    if (edgeStates[i] == 1) {  // manifold edge
                        int v0 = indices_[faceIdx * 3 + i];
                        int v1 = indices_[faceIdx * 3 + (i + 1) % 3];
                        Edge edge = makeEdge(v0, v1);
                        
                        // 이 edge를 공유하는 모든 face들 가져오기 (1개만 있어야함)
                        const auto& facesOnEdge = edgeToFaces_.at(edge);
                        for (int neighborFaceIdx : facesOnEdge) {
                            if (neighborFaceIdx != faceIdx) {  // 현재 face 제외
                                neighborFaces.insert(neighborFaceIdx);
                            }
                        }
                    }
                }
                
                // 이웃 face 두 개의 index
                std::vector<int> neighborFaceIndices(neighborFaces.begin(), neighborFaces.end());
                
                // 이웃 face들의 edge 확인
                int boundaryInNeighbors = 0;
                int nonManifoldInNeighbors = 0;
                
                for (int neighborFaceIdx : neighborFaceIndices) {
                    // 이웃 face의 세 edge 확인
                    for (int i = 0; i < 3; i++) {
                        int v0 = indices_[neighborFaceIdx * 3 + i];
                        int v1 = indices_[neighborFaceIdx * 3 + (i + 1) % 3];
                        Edge edge = makeEdge(v0, v1);
                        
                        // edge가 존재하는지 확인
                        auto it = edgeToFaces_.find(edge);
                        if (it != edgeToFaces_.end()) {
                            int edgeFaceCount = it->second.size();
                            if (edgeFaceCount == 1) {
                                boundaryInNeighbors++;
                            } else if (edgeFaceCount > 2) {
                                nonManifoldInNeighbors++;
                            }
                        }
                    }
                }
                
                if (boundaryInNeighbors > 0) {
                    shouldAdd = true;
                    caseCount++;
                }
            }
            
            if (shouldAdd) {
                boundaryFaces_[0].push_back(faceIdx);
            }
        } else if (numBoundaryEdges == 2) {
            bool shouldAdd = false;
            if (nonManifoldEdge == 1) {
                shouldAdd = true;
            } else {
            // 이웃 face 두 개의 index 가져오기
                std::unordered_set<int> neighborFaces;
                    
                // manifold edge 1개 찾기
                for (int i = 0; i < 3; i++) {
                    if (edgeStates[i] == 1) {  // manifold edge
                        int v0 = indices_[faceIdx * 3 + i];
                        int v1 = indices_[faceIdx * 3 + (i + 1) % 3];
                        Edge edge = makeEdge(v0, v1);
                        
                        // 이 edge를 공유하는 모든 face들 가져오기 (1개만 있어야함)
                        const auto& facesOnEdge = edgeToFaces_.at(edge);
                        for (int neighborFaceIdx : facesOnEdge) {
                            if (neighborFaceIdx != faceIdx) {  // 현재 face 제외
                                neighborFaces.insert(neighborFaceIdx);
                            }
                        }
                    }
                }
                
                // 이웃 face 두 개의 index
                std::vector<int> neighborFaceIndices(neighborFaces.begin(), neighborFaces.end());
                
                // 이웃 face들의 edge 확인
                int boundaryInNeighbors = 0;
                int nonManifoldInNeighbors = 0;
                
                for (int neighborFaceIdx : neighborFaceIndices) {
                    // 이웃 face의 세 edge 확인
                    for (int i = 0; i < 3; i++) {
                        int v0 = indices_[neighborFaceIdx * 3 + i];
                        int v1 = indices_[neighborFaceIdx * 3 + (i + 1) % 3];
                        Edge edge = makeEdge(v0, v1);
                        
                        // edge가 존재하는지 확인
                        auto it = edgeToFaces_.find(edge);
                        if (it != edgeToFaces_.end()) {
                            int edgeFaceCount = it->second.size();
                            if (edgeFaceCount == 1) {
                                boundaryInNeighbors++;
                            } else if (edgeFaceCount > 2) {
                                nonManifoldInNeighbors++;
                            }
                        }
                    }
                }
                
                if (boundaryInNeighbors > 0) {
                    shouldAdd = true;
                }
            }
            
            if (shouldAdd) {
                boundaryFaces_[1].push_back(faceIdx);
            }
            // todo: adjacent face가 boundary edge를 두개 가진 경우 삭제
        } else if (numBoundaryEdges == 3) {
            boundaryFaces_[2].push_back(faceIdx);
        }
    }
    
    std::cout << "Boundary faces:\n"
              << "  1 boundary edge: " << boundaryFaces_[0].size() << "\n"
              << "  2 boundary edges: " << boundaryFaces_[1].size() << "\n"
              << "  3 boundary edges: " << boundaryFaces_[2].size() << "\n"
              << "  one boundary faces: " << oneBoundaryFace << "\n"
              << "  cases counnt " << caseCount 
              << std::endl;
}

void Mesh::buildValence() {
    // Initialize valence vector with zeros
    vertexValences_.clear();
    vertexValences_.resize(vertices_.size(), 0);
    
    // For each vertex, collect all neighboring vertices using a set
    std::vector<std::unordered_set<int>> neighbors(vertices_.size());
    
    // Iterate through all faces
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
    
    // Optional: print statistics
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
    // 임시로 remove target faces 담을 벡터
    nonManifoldToRemove.clear();

    std::vector<int> facesToRemove;
    // 빠른 조회를 위한 set
    std::unordered_set<int> facesToRemoveSet;

    int skippedCount = 0;
    int manifoldCount = 0;

    // edge를 순회
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

        nonManifoldToRemove.insert(faceToRemove);
        facesToRemove.push_back(faceToRemove);
        facesToRemoveSet.insert(faceToRemove);

    }

    // removeFaces(facesToRemove);
    std::cout << "Skipped count (no min): " << skippedCount << std::endl;

    std::cout << "Skipped count (mainfold): " << manifoldCount << std::endl;
    std::cout << "Removed faces: " << nonManifoldToRemove.size() << std::endl;
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
    
    // Build set for fast lookup
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
    

    buildEdgeFaceAdjacency();
    std::vector<std::vector<int>> components = getComponents(); 
    if (components.size() > 1) {
        removeRestComponents(components);
    } else {
        analyzeMesh();
        findBoundaryFaces();
        buildValence();
        findNonManifoldFacesToRemove();
    }
}

void Mesh::removeBoundaryFaces(int boundarySelection) {
    if (boundarySelection < 0 || boundarySelection > 2) {
        return;
    }
    
    const std::vector<int>& facesToRemoveVec = boundaryFaces_[boundarySelection];
    removeFaces(facesToRemoveVec);
    std::cout << " (boundary selection: " << boundarySelection << ")" << std::endl;
}

void Mesh::removeNonManifoldFaces() {
    std::cout << "◦ Processing: removing non-manifold faces ..." << std::endl;
    if (nonManifoldWith4Faces_ >= 0 && nonManifoldWith3Faces_ > 10000) {
        if (nonManifoldToRemove.empty()) {
            std::cout << "• Failed: no non-manifold faces to remove" << std::endl;
        } else {
            std::vector<int> facesToRemoveVec(nonManifoldToRemove.begin(), nonManifoldToRemove.end());
            removeFaces(facesToRemoveVec);
            std::cout << "• Completed: non-manifold faces removed: " << facesToRemoveVec.size() << " faces" << std::endl;
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
