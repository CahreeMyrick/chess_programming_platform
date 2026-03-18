# Chess Training Platform

## Structure

- `services/` - backend and app services
- `libs/` - shared libraries
- `docs/` - documentation

## Current status
- `services/chess-engine/` benchmarking completed - run instructions below
- `services/puzzle-analysis/`  not added yet
- `services/pdf-scanner/`  not added yet
- `services/camera-scanner/`  not added yet

- - `docs/` - I'll start adding writeups on design decisions, these will probably be pretty informal / blog or journal style
  but we'll see.

## Benchmarking results
### Architecture Experiment: Multiple Board Backends

This engine supports three interchangeable board representations:

- **ArrayBoard** — 1D array of piece enums (baseline)
- **PointerBoard** — object-oriented design using polymorphic pieces
- **Bitboard** — 64-bit bitwise representation

All higher-level engine components (move generation, rules, evaluation, search)
are shared across backends via a common interface.

---

### Results (Depth = 3, Averaged)

| Backend       | Avg Time (ms) |
|--------------|--------------|
| ArrayBoard    | ~20 ms       |
| PointerBoard  | ~135 ms      |
| Bitboard      | ~22 ms       |

---

## Key Findings

- **Bitboard performance is comparable to ArrayBoard**, not faster.

- **Conclusion:**  
  Bitboards alone do **not** provide a performance advantage unless the *entire engine pipeline* (especially move generation and attack detection) is redesigned to leverage bitwise parallelism.


---

## Build & Run

### Prerequisites

- C++17 or later
- CMake ≥ 3.15
- A C++ compiler:
  - macOS: `clang++`
  - Linux: `g++`
  - Windows: MSVC or MinGW

---

### Build

Clone the repository and build using CMake:

```bash
git clone <your-repo-url>
cd chess-training-platform

mkdir -p build
cd build

cmake ..
cmake --build .
```

### Executables
Runs the main engine (move generation, search, etc.)
```./chess_engine```

Runs performance comparisons across board backends:
```./chess_benchmark```


  
