#pragma once

#include <vector>

enum class BindingPoint {
    State = 0,
    Known,
    Residuals,
    SearchDirections,
    IndexMap,
    SurfaceValues,

    MatrixValues,
    MatrixIndices,
    Vector,
};

struct MatrixTarget {
    unsigned int values;
    unsigned int indices;
};

class GPUConjGrad {
public:
    unsigned int N;
    unsigned int M;
    unsigned int total_nodes;

    unsigned int state;
    unsigned int known;
    unsigned int residuals;
    unsigned int search_directions;
    unsigned int idx_map;

    MatrixTarget A1;
    MatrixTarget A2;

    unsigned int x1;
    unsigned int x2;

    GPUConjGrad(unsigned int N, unsigned int M, unsigned int total_nodes, const std::vector<unsigned int>& idx_map);

    void init(const std::vector<unsigned int>& idx_map);
    void init_matrix_buffers();
    void init_vector_buffers();
    void solve(MatrixTarget matrix_target, unsigned int x, unsigned int b);

    void loadState();
    void loadSparseMatrix(MatrixTarget matrix_target, const std::vector<std::vector<float>>& data, const std::vector<std::vector<unsigned int>>& indices);
    void loadVector(unsigned int vector_target, const std::vector<float>& vector);
private:
};