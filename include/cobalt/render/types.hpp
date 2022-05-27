#pragma once

namespace cobalt {

/// @brief Shader stage type
enum class shader_stage {
    /// @brief Vertex stage, a shader is running on vertices
    vertex,

    /// @brief Fragment (pixel) stage, a shader is running on every pixel
    fragment,
};

/// @brief Buffer type type
enum class buffer_type {
    /// @brief Vertex buffer type
    vertex,

    /// @brief Index buffer type
    index,
};

/// @brief Buffer usage type
enum class buffer_usage {
    /// @brief Static buffer. Once buffer is created, its data is unchanged
    static_usage,

    /// @brief Dynamic buffer. The data in the buffer might be changed
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

} // namespace cobalt