#version 460
layout (local_size_x = 1024) in;

layout (binding = 0, std430 ) buffer ssbo0 {float buffer_input[];};
layout (binding = 1, std430 ) buffer ssbo1 {float buffer_result[];};

uniform int buffer_size;
uniform bool first_pass;

shared float shared_data[1024];

void main() {
    int globalID = int(gl_GlobalInvocationID.x);
    int localID = int(gl_LocalInvocationID.x);

    float result = 0.0;
    if (gl_GlobalInvocationID.x < buffer_size) {
        if (first_pass) {
            result = buffer_input[globalID] * buffer_input[globalID];
        } else {
            result = buffer_result[globalID];
        }
    }
    shared_data[localID] = result;
    barrier();

    for (int i = int(gl_WorkGroupSize.x) / 2; i > 0; i >>= 1) {
        if (gl_LocalInvocationID.x < i) {
            shared_data[localID] += shared_data[localID + i];
        }
        barrier();
    }

    if (localID == 0) {
        buffer_result[gl_WorkGroupID.x] = shared_data[localID];
    }
}