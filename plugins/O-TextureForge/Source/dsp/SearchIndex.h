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

    SearchIndex.h
    v1.8.0: a KD-tree built with steered descriptor weights.

    The weights are applied by pre-scaling each descriptor dimension by sqrt(w)
    (research granular-synthesis-2026-update §3.5), so the tree stays plain
    Euclidean and a query costs exactly what it did. A weight change therefore
    means a new tree: build() allocates and runs on a worker thread, the
    processor publishes the result like a corpus (raw pointer for the audio
    thread, parked and reaped on the message thread), and the audio thread only
    ever reads it.

    The index owns a reference to the corpus whose grains the tree reads, so the
    tree can never outlive its points. The audio thread uses it only while it
    belongs to the corpus being played (corpus.get() == the block's corpus);
    otherwise the corpus's own default-weighted tree is used.

    Not built at all while every weight is at 100 %: the default path is the
    corpus's own tree, exactly as in v1.7.0.

  ==============================================================================
*/

#pragma once

#include "SharedCorpus.h"
#include "KDTreeSearch.h"
#include <memory>

struct SearchIndex
{
    std::shared_ptr<SharedCorpus> corpus;   // keeps the tree's points alive
    WeightMults mults {};                   // the multipliers it was built with
    std::unique_ptr<KDTreeSearch> tree;
    uint64_t generation = 0;                // unique per build (candidate-cache key)

    // Allocates; never on the audio thread.
    static std::shared_ptr<SearchIndex> build(std::shared_ptr<SharedCorpus> corpus,
                                              const WeightMults& mults, uint64_t generation)
    {
        auto idx = std::make_shared<SearchIndex>();
        idx->mults = mults;
        idx->generation = generation;
        if (corpus != nullptr && !corpus->grains.empty())
            idx->tree = std::make_unique<KDTreeSearch>(corpus->grains, steeredDescriptorScale(mults));
        idx->corpus = std::move(corpus);
        return idx;
    }

    static bool isDefault(const WeightMults& m) noexcept
    {
        for (float v : m)
            if (v != 1.0f)
                return false;
        return true;
    }
};
