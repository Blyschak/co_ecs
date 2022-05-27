#pragma once

#include <cobalt/render/frame_buffer.hpp>

#include <glad/glad.h>

namespace cobalt {

class gl_frame_buffer {
public:
    static constexpr auto max_framebuffer_size = 8192;

    gl_frame_buffer(const frame_buffer_spec& spec) {
        for (auto spec : spec.attachments.attachments) {
            if (spec.is_depth_texture_format()) {
                _depth_attachment_spec = spec;
            } else {
                _color_attachments_specs.emplace_back(spec);
            }
        }
    }

    void invalidate() {
        if (_framebuffer) {
            glDeleteBuffers(1, &_framebuffer);
            glDeleteTextures(_color_attachments.size(), _color_attachments.data());
            glDeleteTextures(1, &_depth_attachment);

            _color_attachments.clear();
            _depth_attachment = 0;
        }

        glCreateFramebuffers(1, &_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

        bool multisample = _specification.samples > 1;

        if (!_color_attachments_specs.empty()) {
            _color_attachments.resize(_color_attachments_specs.size());
            glCreateTextures(multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                _color_attachments.size(),
                _color_attachments.data());

            for (size_t i = 0; i < _color_attachments.size(); i++) {
                glBindTexture(multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, _color_attachments[i]);
                // switch (_color_attachments_specs[i].texture_format) {
                // case frame_buffer_texture_format::rgba8:
                //     attach_color_texture(_color_attachments[i],
                //         _specification.samples,
                //         GL_RGBA8,
                //         GL_RGBA,
                //         _specification.width,
                //         _specification.height,
                //         i);
                //     break;
                // case frame_buffer_texture_format::red_integer:
                //     att(_color_attachments[i],
                //         _specification.samples,
                //         GL_R32I,
                //         GL_RED_INTEGER,
                //         _specification.width,
                //         _specification.height,
                //         i);
                //     break;
                // }
            }
        }
    }

private:
    GLuint _framebuffer{};
    frame_buffer_spec _specification;

    std::vector<frame_buffer_texture_spec> _color_attachments_specs;
    frame_buffer_texture_spec _depth_attachment_spec{ frame_buffer_texture_format::none };

    std::vector<GLuint> _color_attachments;
    GLuint _depth_attachment{};
};

} // namespace cobalt