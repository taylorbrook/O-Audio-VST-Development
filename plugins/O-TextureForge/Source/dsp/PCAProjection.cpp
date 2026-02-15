/*
  ==============================================================================

    PCAProjection.cpp
    PCA implementation using Eigen

  ==============================================================================
*/

#include "PCAProjection.h"
#include <Eigen/Dense>
#include <algorithm>
#include <cmath>

void PCAProjection::compute(const std::vector<GrainMetadata>& grains,
                            std::vector<float>& outX,
                            std::vector<float>& outY)
{
    const int N = static_cast<int>(grains.size());
    constexpr int D = 19;

    if (N < 2)
    {
        outX.assign(static_cast<size_t>(N), 0.5f);
        outY.assign(static_cast<size_t>(N), 0.5f);
        return;
    }

    // Build Eigen matrix (N rows x 19 cols)
    Eigen::MatrixXd data(N, D);
    for (int i = 0; i < N; ++i)
        for (int d = 0; d < D; ++d)
            data(i, d) = static_cast<double>(grains[i].descriptors[d]);

    // Center data (subtract column means)
    Eigen::VectorXd mean = data.colwise().mean();
    data.rowwise() -= mean.transpose();

    // Compute covariance matrix (19x19 — tiny, instant)
    Eigen::MatrixXd cov = (data.transpose() * data) / (N - 1);

    // Eigenvalue decomposition
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(cov);

    // Take top 2 eigenvectors (highest eigenvalues are last columns)
    Eigen::MatrixXd pc = solver.eigenvectors().rightCols(2);
    // Reverse so PC1 is first column
    pc.col(0).swap(pc.col(1));

    // Project data onto 2 principal components
    Eigen::MatrixXd projected = data * pc;  // N x 2

    // Normalize to [0, 1] range
    double minX = projected.col(0).minCoeff();
    double maxX = projected.col(0).maxCoeff();
    double minY = projected.col(1).minCoeff();
    double maxY = projected.col(1).maxCoeff();
    double rangeX = std::max(maxX - minX, 1e-10);
    double rangeY = std::max(maxY - minY, 1e-10);

    outX.resize(static_cast<size_t>(N));
    outY.resize(static_cast<size_t>(N));
    for (int i = 0; i < N; ++i)
    {
        outX[i] = static_cast<float>((projected(i, 0) - minX) / rangeX);
        outY[i] = static_cast<float>((projected(i, 1) - minY) / rangeY);
    }
}
