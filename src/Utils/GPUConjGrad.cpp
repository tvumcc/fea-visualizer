#include <glad/glad.h>

#include "Utils/GPUConjGrad.hpp"

GPUConjGrad::GPUConjGrad(unsigned int N, unsigned int M, unsigned int total_nodes, const std::vector<unsigned int>& idx_map) {
    this->N = N;
    this->M = M;
    this->total_nodes = total_nodes;

    init(idx_map);
}

void GPUConjGrad::init(const std::vector<unsigned int>& idx_map) {
    glGenBuffers(1, &this->state);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->state);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, (N + 7) * sizeof(float), NULL, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::State), this->state);

    glGenBuffers(1, &this->known);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->known);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Known), this->known);

    glGenBuffers(1, &this->residuals);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->residuals);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Residuals), this->residuals);

    glGenBuffers(1, &this->search_directions);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->search_directions);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::SearchDirections), this->search_directions);

    glGenBuffers(1, &this->idx_map);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->idx_map);
    glBufferData(GL_SHADER_STORAGE_BUFFER, idx_map.size() * sizeof(unsigned int), idx_map.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::IndexMap), this->idx_map);

    init_matrix_buffers();
    init_vector_buffers();
    loadState();
}

void GPUConjGrad::init_matrix_buffers() {
    glGenBuffers(1, &this->A1.values);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->A1.values);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &this->A1.indices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->A1.indices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(unsigned int), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &this->A2.values);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->A2.values);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &this->A2.indices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->A2.indices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(unsigned int), NULL, GL_STATIC_DRAW);
}

void GPUConjGrad::init_vector_buffers() {
    glGenBuffers(1, &this->x1);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->x1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), NULL, GL_STATIC_DRAW);
    
    glGenBuffers(1, &this->x2);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->x2);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * sizeof(float), NULL, GL_STATIC_DRAW);
}

void GPUConjGrad::solve(MatrixTarget matrix_target, unsigned int x, unsigned int b) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::MatrixValues), matrix_target.values);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::MatrixIndices), matrix_target.indices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Vector), x);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Known), b);
}

void GPUConjGrad::loadState() {
    float data[] = {N, M, total_nodes};

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->state);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(float), 3 * sizeof(unsigned int), data);
}

void GPUConjGrad::loadSparseMatrix(MatrixTarget matrix_target, const std::vector<std::vector<float>>& data, const std::vector<std::vector<unsigned int>>& indices) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, matrix_target.values);      
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), data.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, matrix_target.indices);      
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), indices.data(), GL_DYNAMIC_DRAW);
}

void GPUConjGrad::loadVector(unsigned int vector_target, const std::vector<float>& vector) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vector_target);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vector.size() * sizeof(float), vector.data(), GL_DYNAMIC_DRAW);
}