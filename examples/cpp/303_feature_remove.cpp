// Copyright 2024-present the vsag project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vsag/vsag.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

std::vector<float>
LoadFvecs(const std::string& path, int64_t& count, int64_t& dim) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open " + path);
    }

    int32_t file_dim = 0;
    input.read(reinterpret_cast<char*>(&file_dim), sizeof(file_dim));
    if (!input || file_dim <= 0) {
        throw std::runtime_error("invalid fvecs file: " + path);
    }

    input.seekg(0, std::ios::end);
    const auto bytes = input.tellg();
    const auto record_bytes =
        static_cast<std::streamoff>(sizeof(int32_t) + file_dim * sizeof(float));
    if (bytes <= 0 || bytes % record_bytes != 0) {
        throw std::runtime_error("invalid fvecs size: " + path);
    }

    count = static_cast<int64_t>(bytes / record_bytes);
    dim = file_dim;
    std::vector<float> vectors(static_cast<size_t>(count * dim));
    input.seekg(0, std::ios::beg);
    for (int64_t i = 0; i < count; ++i) {
        int32_t current_dim = 0;
        input.read(reinterpret_cast<char*>(&current_dim), sizeof(current_dim));
        if (current_dim != file_dim) {
            throw std::runtime_error("inconsistent fvecs dimensions: " + path);
        }
        input.read(reinterpret_cast<char*>(vectors.data() + i * dim),
                   static_cast<std::streamsize>(dim * sizeof(float)));
    }
    if (!input) {
        throw std::runtime_error("truncated fvecs file: " + path);
    }
    return vectors;
}

}  // namespace

int
main(int argc, char** argv) {
    try {
        vsag::init();
        const std::string dataset_dir = argc > 1 ? argv[1] : "/workspace/datasets/sift/10k";

        int64_t num_vectors = 0;
        int64_t dim = 0;
        auto vectors = LoadFvecs(dataset_dir + "/sift_base.fvecs", num_vectors, dim);
        int64_t num_queries = 0;
        int64_t query_dim = 0;
        auto queries = LoadFvecs(dataset_dir + "/sift_query.fvecs", num_queries, query_dim);
        if (dim != query_dim || num_queries == 0) {
            throw std::runtime_error("base/query dimensions do not match");
        }

        std::vector<int64_t> ids(static_cast<size_t>(num_vectors));
        for (int64_t i = 0; i < num_vectors; ++i) {
            ids[static_cast<size_t>(i)] = i;
        }
        auto base = vsag::Dataset::Make();
        base->NumElements(num_vectors)
            ->Dim(dim)
            ->Ids(ids.data())
            ->Float32Vectors(vectors.data())
            ->Owner(false);

        const auto build_parameters = std::string(R"({
            "dtype": "float32",
            "metric_type": "l2",
            "dim": )") +
                                      std::to_string(dim) + R"(,
            "index_param": {
                "base_quantization_type": "sq8",
                "max_degree": 16,
                "ef_construction": 100,
                "support_remove": true,
                "remove_repair_k": 3
            }
        })";
        auto create_result = vsag::Factory::CreateIndex("hgraph", build_parameters);
        if (!create_result.has_value()) {
            throw std::runtime_error("create failed: " + create_result.error().message);
        }
        auto index = create_result.value();
        auto build_result = index->Build(base);
        if (!build_result.has_value()) {
            throw std::runtime_error("build failed: " + build_result.error().message);
        }

        auto query = vsag::Dataset::Make();
        query->NumElements(1)->Dim(dim)->Float32Vectors(queries.data())->Owner(false);
        const std::string search_parameters = R"({"hgraph":{"ef_search":100}})";
        auto before = index->KnnSearch(query, 10, search_parameters);
        if (!before.has_value() || before.value()->GetDim() < 5) {
            throw std::runtime_error("initial search did not return enough neighbors");
        }

        const int64_t scalar_id = before.value()->GetIds()[0];
        std::vector<int64_t> batch_ids;
        for (int64_t i = 1; i < 5; ++i) {
            batch_ids.emplace_back(before.value()->GetIds()[i]);
        }
        auto scalar_result = index->Remove(scalar_id);
        auto batch_result = index->Remove(batch_ids);
        if (!scalar_result.has_value() || !scalar_result.value() || !batch_result.has_value() ||
            !batch_result.value()) {
            throw std::runtime_error("Merit Remove returned failure");
        }

        auto after = index->KnnSearch(query, 10, search_parameters);
        if (!after.has_value()) {
            throw std::runtime_error("search after Remove failed: " + after.error().message);
        }
        std::unordered_set<int64_t> removed(batch_ids.begin(), batch_ids.end());
        removed.insert(scalar_id);
        for (int64_t i = 0; i < after.value()->GetDim(); ++i) {
            if (removed.count(after.value()->GetIds()[i]) != 0) {
                throw std::runtime_error("a removed ID was returned by search");
            }
        }

        std::cout << "PASS: Merit HGraph Remove on Sift10k (1 scalar + " << batch_ids.size()
                  << " batched deletions)" << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << std::endl;
        return 1;
    }
}
