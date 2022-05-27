#pragma once

#include <cstdint>
#include <vector>

namespace cobalt {

enum class frame_buffer_texture_format {
    none = 0,

    // Color
    rgba8,
    red_integer,

    // Depth/stencil
    depth24stencil8,

    // Defaults
    depth = depth24stencil8
};

struct frame_buffer_texture_spec {
    frame_buffer_texture_spec() = default;
    frame_buffer_texture_spec(frame_buffer_texture_format format) : texture_format(format) {
    }

    frame_buffer_texture_format texture_format = frame_buffer_texture_format::none;


    [[nodiscard]] bool is_depth_texture_format() const noexcept {
        switch (texture_format) {
        case frame_buffer_texture_format::depth24stencil8:
            return true;
        }

        return false;
    }

    // TODO: filtering/wrap
};

struct frame_buffer_attachment_spec {
    frame_buffer_attachment_spec() = default;
    frame_buffer_attachment_spec(std::initializer_list<frame_buffer_texture_spec> attachments) :
        attachments(attachments) {
    }

    std::vector<frame_buffer_texture_spec> attachments;
};

struct frame_buffer_spec {
    uint32_t width{}, height{};
    frame_buffer_attachment_spec attachments{};
    uint32_t samples{ 1 };

    bool swap_chain_target{ false };
};

} // namespace cobalt