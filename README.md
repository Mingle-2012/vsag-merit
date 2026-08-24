# Merit: HGraph Deletion Repair

This artifact is based on Ant Group VSAG commit
`6cf34b908dc72859c1a111fefe831d58dd704240`. It contains the Merit deletion
repair implementation for VSAG HGraph in both the single-ID and batched APIs:

```cpp
bool HGraph::Remove(int64_t id);
bool HGraph::Remove(const std::vector<int64_t>& ids);
```

The original VSAG documentation is preserved in
[ORIGINAL_README.md](ORIGINAL_README.md).

## Build

Requirements are the same as upstream VSAG: a C++17 compiler, CMake, Git, and
the build dependencies fetched by CMake.

```bash
cmake -S . -B build \
  -DENABLE_EXAMPLES=ON \
  -DENABLE_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target 303_feature_remove -j
```

## Test Merit on Sift10k

The example expects `sift_base.fvecs` and `sift_query.fvecs` in the dataset
directory. It builds an HGraph with deletion enabled, invokes one scalar Remove
and one batched Remove, then verifies that none of the deleted IDs occur in the
post-deletion search results.

```bash
./build/examples/cpp/303_feature_remove /workspace/datasets/sift/10k
```

A successful run prints:

```text
PASS: Merit HGraph Remove on Sift10k (1 scalar + 4 batched deletions)
```

`remove_repair_k` controls how many already-connected vertices each new Merit
repair vertex links to. The example uses `remove_repair_k: 3`.

## License

This repository is distributed under the Apache License 2.0. See
[LICENSE](LICENSE).
