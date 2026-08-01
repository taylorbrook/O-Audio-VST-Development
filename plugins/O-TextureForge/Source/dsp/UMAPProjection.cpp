/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    UMAPProjection.cpp
    UMAP implementation using umappp

  ==============================================================================
*/

#include "UMAPProjection.h"
#include <umappp/umappp.hpp>
#include <knncolle/knncolle.hpp>
#include <Eigen/Dense>
#include <algorithm>
#include <memory>
#include <thread>

void initEigenOnMainThread()
{
    // Trigger Eigen's manage_caching_sizes static initialization on the main thread.
    // This forces __cxa_guard_acquire to run in the arm64e host's main thread context
    // (where PAC authentication works), rather than on a background pthread created
    // by arm64 plugin code (where PAC traps occur).
    Eigen::setCpuCacheSizes(131072, 4194304, 0);
}

void UMAPProjection::compute(const std::vector<GrainMetadata>& grains,
                             std::vector<float>& outX,
                             std::vector<float>& outY,
                             ProgressCallback progressCallback,
                             std::function<bool()> shouldCancel)
{
    const int N = static_cast<int>(grains.size());
    constexpr int D = 19;

    if (N < 15)
    {
        outX.assign(static_cast<size_t>(N), 0.5f);
        outY.assign(static_cast<size_t>(N), 0.5f);
        return;
    }

    // Convert row-major grain descriptors to column-major for umappp
    // umappp expects: ndim rows x nobs columns (column-major)
    std::vector<double> data(static_cast<size_t>(D) * N);
    for (int i = 0; i < N; ++i)
        for (int d = 0; d < D; ++d)
            data[static_cast<size_t>(d) + static_cast<size_t>(i) * D] = static_cast<double>(grains[i].descriptors[d]);

    // 2D embedding output
    std::vector<double> embedding(static_cast<size_t>(2) * N, 0.0);

    // Configure UMAP
    umappp::Options opt;
    opt.num_neighbors = 15;
    opt.min_dist = 0.1;
    opt.num_threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 2);
    opt.num_threads_optimize = 1;   // serial optimization (umappp v3.3.0 threading Options migration)
    opt.num_threads_spectral = 1;   // deterministic irlba spectral init (FP results vary with thread count)
    opt.initialize_method = umappp::InitializeMethod::SPECTRAL;

    // Build nearest-neighbor index and initialize UMAP
    auto metric = std::make_shared<knncolle::EuclideanDistance<double, double>>();
    knncolle::VptreeBuilder<int, double, double> builder(metric);
    auto status = umappp::initialize<int, double>(
        static_cast<std::size_t>(D),
        N,
        data.data(),
        builder,
        static_cast<std::size_t>(2),
        embedding.data(),
        opt
    );

    // Run epochs in batches for progress reporting
    int totalEpochs = status.num_epochs();
    int batchSize = 50;

    for (int epoch = batchSize; epoch <= totalEpochs; epoch += batchSize)
    {
        if (shouldCancel && shouldCancel())
            return;

        status.run(embedding.data(), epoch);

        if (progressCallback)
        {
            float progress = static_cast<float>(epoch) / static_cast<float>(totalEpochs);
            progressCallback(progress);
        }
    }

    // Final run to complete remaining epochs
    status.run(embedding.data());

    // Normalize embedding to [0, 1]
    double minX = embedding[0], maxX = embedding[0];
    double minY = embedding[1], maxY = embedding[1];

    for (int i = 0; i < N; ++i)
    {
        double x = embedding[static_cast<size_t>(i) * 2];
        double y = embedding[static_cast<size_t>(i) * 2 + 1];
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }

    double rangeX = std::max(maxX - minX, 1e-10);
    double rangeY = std::max(maxY - minY, 1e-10);

    outX.resize(static_cast<size_t>(N));
    outY.resize(static_cast<size_t>(N));
    for (int i = 0; i < N; ++i)
    {
        outX[i] = static_cast<float>((embedding[static_cast<size_t>(i) * 2] - minX) / rangeX);
        outY[i] = static_cast<float>((embedding[static_cast<size_t>(i) * 2 + 1] - minY) / rangeY);
    }
}
