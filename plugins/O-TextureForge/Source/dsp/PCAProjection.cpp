/*
  ==============================================================================

    PCAProjection.cpp
    PCA implementation — manual covariance + power iteration
    (Avoids Eigen's static guard variable which triggers PAC trap
     in arm64e AU hosting processes)

  ==============================================================================
*/

#include "PCAProjection.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <array>

namespace
{
    constexpr int D = 19;

    using Vec19 = std::array<float, D>;
    using Mat19 = std::array<std::array<float, D>, D>;

    // Matrix-vector multiply: out = M * v
    void matVecMul(const Mat19& M, const Vec19& v, Vec19& out)
    {
        for (int i = 0; i < D; ++i)
        {
            float sum = 0.0f;
            for (int j = 0; j < D; ++j)
                sum += M[i][j] * v[j];
            out[i] = sum;
        }
    }

    // Dot product
    float dot(const Vec19& a, const Vec19& b)
    {
        float sum = 0.0f;
        for (int i = 0; i < D; ++i)
            sum += a[i] * b[i];
        return sum;
    }

    // Normalize vector in-place, returns length
    float normalize(Vec19& v)
    {
        float len = std::sqrt(dot(v, v));
        if (len > 1e-12f)
            for (int i = 0; i < D; ++i)
                v[i] /= len;
        return len;
    }

    // Power iteration: find dominant eigenvector of symmetric matrix M
    // Returns eigenvalue
    float powerIteration(const Mat19& M, Vec19& eigenvector, int maxIter = 100)
    {
        // Initialize with a non-degenerate vector
        for (int i = 0; i < D; ++i)
            eigenvector[i] = 1.0f + 0.1f * static_cast<float>(i);
        normalize(eigenvector);

        Vec19 temp;
        float eigenvalue = 0.0f;

        for (int iter = 0; iter < maxIter; ++iter)
        {
            matVecMul(M, eigenvector, temp);
            eigenvalue = normalize(temp);

            // Check convergence
            float diff = 0.0f;
            for (int i = 0; i < D; ++i)
                diff += std::abs(temp[i] - eigenvector[i]);
            eigenvector = temp;

            if (diff < 1e-7f)
                break;
        }

        return eigenvalue;
    }

    // Deflate matrix: M = M - eigenvalue * v * v^T
    void deflate(Mat19& M, const Vec19& v, float eigenvalue)
    {
        for (int i = 0; i < D; ++i)
            for (int j = 0; j < D; ++j)
                M[i][j] -= eigenvalue * v[i] * v[j];
    }
}

void PCAProjection::compute(const std::vector<GrainMetadata>& grains,
                            std::vector<float>& outX,
                            std::vector<float>& outY)
{
    const int N = static_cast<int>(grains.size());

    if (N < 2)
    {
        outX.assign(static_cast<size_t>(N), 0.5f);
        outY.assign(static_cast<size_t>(N), 0.5f);
        return;
    }

    // Step 1: Compute column means
    Vec19 mean;
    mean.fill(0.0f);
    for (int i = 0; i < N; ++i)
        for (int d = 0; d < D; ++d)
            mean[d] += grains[i].descriptors[d];
    for (int d = 0; d < D; ++d)
        mean[d] /= static_cast<float>(N);

    // Step 2: Compute covariance matrix (19x19, symmetric)
    Mat19 cov;
    for (auto& row : cov)
        row.fill(0.0f);

    for (int i = 0; i < N; ++i)
    {
        Vec19 centered;
        for (int d = 0; d < D; ++d)
            centered[d] = grains[i].descriptors[d] - mean[d];

        for (int r = 0; r < D; ++r)
            for (int c = r; c < D; ++c)
                cov[r][c] += centered[r] * centered[c];
    }

    // Symmetrize and normalize
    float invN = 1.0f / static_cast<float>(N - 1);
    for (int r = 0; r < D; ++r)
    {
        for (int c = r; c < D; ++c)
        {
            cov[r][c] *= invN;
            cov[c][r] = cov[r][c];
        }
    }

    // Step 3: Find top 2 eigenvectors via power iteration + deflation
    Vec19 pc1, pc2;
    float ev1 = powerIteration(cov, pc1);
    deflate(cov, pc1, ev1);
    powerIteration(cov, pc2);

    // Step 4: Project data onto PC1 and PC2
    outX.resize(static_cast<size_t>(N));
    outY.resize(static_cast<size_t>(N));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (int i = 0; i < N; ++i)
    {
        float px = 0.0f, py = 0.0f;
        for (int d = 0; d < D; ++d)
        {
            float centered = grains[i].descriptors[d] - mean[d];
            px += centered * pc1[d];
            py += centered * pc2[d];
        }
        outX[i] = px;
        outY[i] = py;
        minX = std::min(minX, px);
        maxX = std::max(maxX, px);
        minY = std::min(minY, py);
        maxY = std::max(maxY, py);
    }

    // Step 5: Normalize to [0, 1]
    float rangeX = std::max(maxX - minX, 1e-10f);
    float rangeY = std::max(maxY - minY, 1e-10f);
    for (int i = 0; i < N; ++i)
    {
        outX[i] = (outX[i] - minX) / rangeX;
        outY[i] = (outY[i] - minY) / rangeY;
    }
}
