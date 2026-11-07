# Efficient In-Place Deletion for Dynamic Graph-Based Approximate Nearest Neighbor Indexes

## Introduction

This repository contains the source code for the paper "MERIT: Efficient In-Place Deletion for Dynamic Graph-Based Approximate Nearest Neighbor Indexes". In the paper, we propose a novel deletion repair algorithm. Merit is designed to efficiently handle deletions in graph-based ANNS and ensure that the graph remains connected and maintains high search performance after deletions.

This repository is based on Ant Group VSAG repository (https://github.com/antgroup/vsag, Commit ID: 6cf34b908dc72859c1a111fefe831d58dd704240). It contains the Merit deletion repair implementation for VSAG HGraph (an HNSW graph variant) in both the single-ID and batched APIs:

```cpp
bool HGraph::Remove(int64_t id);
bool HGraph::Remove(const std::vector<int64_t>& ids);
```

## Before you start

The original VSAG documentation is preserved in [ORIGINAL_README.md](ORIGINAL_README.md). You have to install several dependencies to build the project before running the reproduction experiments, as described in the original VSAG documentation.

## Build

Requirements are the same as upstream VSAG: a C++17 compiler, CMake, Git, and the build dependencies fetched by CMake.

```bash
cmake -S . -B build \
  -DENABLE_EXAMPLES=ON \
  -DENABLE_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --target 303_feature_remove -j
```

## Test Merit on datasets

To test the Merit deletion algorithm on real-world datasets, you can run the example `303_feature_remove`.

Here we provide a table of datasets that can be used to test the Merit deletion algorithm:

| Dataset         | #Points    | #Dimensions | #Queries |
|-----------------|------------|-------------|----------|
| SIFT100M          | 100,000,000  | 128         | 10,000   |
| DEEP100M        | 100,000,000| 96          | 10,000   |
| MSong           | 30,000,000    | 768         | 10,000    |

The example `303_feature_remove` expects `sift_base.fvecs` and `sift_query.fvecs` in the dataset directory. It builds an HGraph with deletion enabled, invokes one scalar Remove and one batched Remove, then verifies that none of the deleted IDs occur in the post-deletion search results.

```bash
./build/examples/cpp/303_feature_remove <your_dataset_dir>
```

### Parameters

Both Remove APIs use the same search-free repair path: propose bidirectional k_r-MST edges, merge them with surviving neighbors, and prune each affected adjacency list once using alpha=1.2.

`remove_repair_k` controls how many already-connected vertices each new Merit repair vertex links to. The example uses `remove_repair_k: 3`.

## Citation

If you use this code in your research, please cite the following paper:

```
@article{wu2026merit,
  title={MERIT: Efficient In-Place Deletion for Dynamic Graph-Based Approximate Nearest Neighbor Indexes},
  author={Wu, Zekai and Jin, Jiabao and Cheng, Peng and Ni, Wangze and Li, Haoyang and Chen, Lei and Yao, Junjie and Song, Jingkuan and Shen, Heng Tao},
  journal={arXiv preprint arXiv:2607.29173},
  year={2026}
}
```

## License

This repository is distributed under the Apache License 2.0. See [LICENSE](LICENSE).
