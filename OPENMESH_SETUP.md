# OpenMesh 프로젝트 설정 요약

## 제거된 파일들

1. **CMakeLists.txt** - 빌드 시스템 파일 (불필요)
2. **.cc 파일들** (~24개) - 소스 파일 (라이브러리에 이미 컴파일됨)

## 유지된 파일들

- **모든 헤더 파일들 (.hh, .h)** - 컴파일 시 필요
  - Mesh 관련 헤더들 (TriMesh_ArrayKernelT.hh 등)
  - IO 관련 헤더들 (MeshIO.hh, OBJReader.hh 등)
  - Geometry 관련 헤더들 (VectorT.hh 등)
  - System, Utils 관련 헤더들

## 왜 모든 헤더가 필요한가?

OpenMesh 헤더들은 서로 강하게 의존적입니다:
- `TriMesh_ArrayKernelT.hh` → `TriMeshT.hh` → `PolyMeshT.hh` → `BaseKernel.hh` → ...
- `MeshIO.hh` → `IOManager.hh` → 모든 Reader/Writer 헤더들

하나라도 빠지면 컴파일 오류가 발생합니다.

## 사용 방법

```cpp
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/IO/MeshIO.hh>

typedef OpenMesh::TriMesh_ArrayKernelT<> MyMesh;

// 메시 로드
MyMesh mesh;
OpenMesh::IO::read_mesh(mesh, "mesh.obj");

// 버텍스 순회
for (auto v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
    auto pos = mesh.point(*v_it);
    // ...
}
```

## 파일 구조

```
dependencies/
├── include/
│   └── OpenMesh/
│       └── Core/          # 헤더 파일들만 (102개)
│           ├── Mesh/
│           ├── IO/
│           ├── Geometry/
│           ├── System/
│           └── Utils/
└── library/
    └── libOpenMeshCore.11.0.dylib  # 컴파일된 라이브러리
```



