# MobileNeRF Mesh Manifold 정제 계획

## 현재 상태 분석

### 현재 구현된 기능
1. **메시 로딩**: OBJ 파일에서 로드, 중복 vertex 제거
2. **Edge-Face Adjacency**: `edgeToFaces_` 맵으로 각 edge가 속한 face들 추적
3. **Boundary Detection**: 
   - Boundary edge (1개 face만 연결)
   - Non-manifold edge (3개 이상 face 연결)
   - Boundary face 분류 (1/2/3 boundary edges)
4. **시각화**: 문제가 있는 face들을 하이라이트
5. **기본 제거**: Boundary face 일괄 제거

### 발견된 문제들
- **Non-manifold edges**: 3개 이상의 face가 하나의 edge를 공유
- **Boundary edges**: 연결되지 않은 edge들
- **Disconnected components**: 분리된 메시 조각들
- **Degenerate faces**: 면적이 0이거나 거의 0인 삼각형
- **Self-intersections**: 겹치는 face들

## Manifold 정제 전략

### Phase 1: 문제 진단 및 분류

#### 1.1 Face Quality Metrics
각 face에 대해 다음 메트릭 계산:
- **Area**: 삼각형 면적 (너무 작으면 degenerate)
- **Aspect Ratio**: 가장 긴 변 / 가장 짧은 변 (1에 가까울수록 좋음)
- **Normal Consistency**: 인접 face들과의 normal 차이
- **Edge Valence**: 각 edge의 연결된 face 개수
- **Boundary Count**: boundary edge 개수

#### 1.2 Face Priority Score
각 face에 우선순위 점수 부여:
```
priority = w1 * area_penalty + 
           w2 * aspect_penalty + 
           w3 * non_manifold_penalty + 
           w4 * boundary_penalty +
           w5 * normal_inconsistency_penalty
```

### Phase 2: Face-by-Face Iterative Refinement

#### 2.1 Iteration Strategy
```
1. 모든 face의 quality score 계산
2. Score가 낮은 순서대로 정렬 (worst first)
3. 각 face에 대해:
   a. 문제 진단 (non-manifold edge? degenerate? isolated?)
   b. 해결 방법 선택:
      - Remove: 완전히 제거
      - Fix: 수정 시도 (vertex merge, edge flip 등)
      - Keep: 유지
   c. 변경사항 적용
   d. Adjacency 정보 업데이트
   e. 다음 iteration을 위해 재계산
4. 수렴할 때까지 반복 (또는 최대 iteration)
```

#### 2.2 Face Processing Options

**Option A: Remove**
- 완전히 제거
- 인접 face들의 boundary 상태 업데이트

**Option B: Vertex Merge**
- Degenerate face의 vertex들을 merge
- Non-manifold edge 해결 시도

**Option C: Edge Flip**
- Non-manifold edge를 다른 방향으로 flip
- Delaunay 조건 개선

**Option D: Split**
- 큰 face를 작은 face들로 분할
- Aspect ratio 개선

### Phase 3: Connectivity Repair

#### 3.1 Non-manifold Edge Resolution
```
for each non-manifold edge (3+ faces):
  1. Face들을 normal 기반으로 그룹화
  2. 가장 일관된 그룹만 유지
  3. 나머지 face들 제거 또는 수정
```

#### 3.2 Boundary Closure
```
for each boundary edge:
  1. 가장 가까운 다른 boundary edge 찾기
  2. 거리가 threshold 이하면 연결
  3. 새로운 face 생성하여 gap 채우기
```

#### 3.3 Component Analysis
```
1. Connected components 찾기
2. 가장 큰 component만 유지 (또는 사용자 선택)
3. 작은 isolated pieces 제거
```

## 구현 계획

### Step 1: Face Quality System 구현

**새로운 클래스/구조체:**
```cpp
struct FaceQuality {
    float area;
    float aspectRatio;
    int boundaryEdgeCount;
    int nonManifoldEdgeCount;
    float normalConsistency;
    float priorityScore;
};

class MeshRefiner {
    // Face quality 계산
    std::vector<FaceQuality> computeFaceQualities(const Mesh& mesh);
    
    // Priority 기반 정렬
    std::vector<int> getSortedFaceIndices(const std::vector<FaceQuality>& qualities);
    
    // Face-by-face 처리
    bool processFace(Mesh& mesh, int faceIdx);
};
```

### Step 2: Face Processing Logic

**각 face 처리 시 고려사항:**
1. **Degenerate Face**: 면적이 거의 0 → 제거
2. **Non-manifold Edge 포함**: 
   - Edge의 face들을 분석
   - 가장 일관된 subset만 유지
3. **Isolated Face**: 3개 edge 모두 boundary → 제거 고려
4. **Poor Aspect Ratio**: Split 또는 neighbor와 merge 고려

### Step 3: Interactive Refinement UI

**UI 기능 추가:**
- Face-by-face navigation (Next/Previous 버튼)
- 현재 face 하이라이트
- Quality metrics 표시
- Action 선택 (Remove/Fix/Keep)
- Batch processing 옵션

### Step 4: Validation & Metrics

**정제 후 검증:**
- Manifold edge 비율
- Boundary edge 개수
- Component 개수
- Average face quality

## 구체적인 구현 단계

### Phase 1: Foundation (현재 → Step 1)
1. `FaceQuality` 구조체 추가
2. `computeFaceQualities()` 구현
3. Quality metrics 시각화

### Phase 2: Basic Refinement (Step 1 → Step 2)
1. Degenerate face 자동 제거
2. Isolated face 제거 옵션
3. Priority 기반 정렬

### Phase 3: Advanced Refinement (Step 2 → Step 3)
1. Non-manifold edge resolution
2. Vertex merge 로직
3. Edge flip 로직

### Phase 4: Interactive Tools (Step 3 → Step 4)
1. Face-by-face navigation
2. Manual selection & action
3. Batch processing

### Phase 5: Optimization (Step 4 → 완료)
1. Iterative refinement loop
2. Convergence detection
3. Performance 최적화

## 데이터 구조 확장 제안

### Mesh 클래스에 추가할 것들:
```cpp
class Mesh {
    // 기존...
    
    // Face quality 정보
    std::vector<FaceQuality> faceQualities_;
    
    // Face-to-face adjacency (각 face의 3개 인접 face)
    std::vector<std::array<int, 3>> faceAdjacency_;  // -1 if boundary
    
    // Vertex-to-face mapping
    std::vector<std::vector<int>> vertexToFaces_;
    
    // Component ID per face
    std::vector<int> faceComponentId_;
    
    // Quality 업데이트
    void updateFaceQualities();
    void updateAdjacency();
    void findConnectedComponents();
};
```

## 우선순위 제안

1. **즉시 구현**: Face quality 계산 및 시각화
2. **단기**: Degenerate/isolated face 자동 제거
3. **중기**: Non-manifold edge resolution
4. **장기**: Interactive face-by-face refinement UI

## 참고 사항

- MobileNeRF 메시는 보통 매우 조밀함 (수백만 face)
- Performance 고려: Incremental update, spatial indexing
- 사용자 피드백: 어떤 face를 제거/수정할지 선택 가능하게
- Undo/Redo 기능 고려

