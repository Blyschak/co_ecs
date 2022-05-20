#pragma once

namespace cobalt::renderer {

/// @brief Shader stage type
enum class shader_stage {
    vertex,
    fragment,
};

/// @brief Buffer type type
enum class buffer_type {
    vertex,
    index,
};

/// @brief Buffer usage type
enum class buffer_usage {
    static_usage,
    dynamic_usage,
};

/// @brief Vertex format type
enum class vertex_format {
    unknown,
    float_dt,
    float2_dt,
    float3_dt,
    float4_dt,
    mat3_dt,
    mat4_dt,
    int_dt,
    int2_dt,
    int3_dt,
    int4_dt,
    bool_dt,
};

} // namespace cobalt::renderer