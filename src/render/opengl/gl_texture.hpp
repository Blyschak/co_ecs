#pragma once

#include <cobalt/core/assert.hpp>

#include <glad/glad.h>

#include <span>

namespace cobalt {

using u8_view = std::span<uint8_t>;

class gl_texture {
public:
    static gl_texture from_binary(u8_view raw, uint32_t width, uint32_t height, int channels) {
        GLuint texture_id;
        auto [internal_format, data_format] = format_from_channels(channels);

        assert_with_message(internal_format & data_format, "Format not supported!");

        glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
        glTextureStorage2D(texture_id, 1, internal_format, width, height);

        glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

        gl_texture texture{ texture_id, internal_format, data_format, width, height };
        texture.set_data(raw);

        return texture;
    }

    void set_data(u8_view raw) {
        uint32_t bpp = _data_format == GL_RGBA ? 4 : 3;
        assert_with_message(raw.size() == _width * _height * bpp, "Data must be entire texture!");
        glTextureSubImage2D(_texture, 0, 0, 0, _width, _height, _data_format, GL_UNSIGNED_BYTE, raw.data());
    }

    void bind(uint32_t slot = 0) {
        glBindTextureUnit(slot, _texture);
    }

    ~gl_texture() {
        glDeleteTextures(1, &_texture);
    }

private:
    gl_texture(GLuint texture, GLenum internal_format, GLenum data_format, uint32_t width, uint32_t height) :
        _texture(texture), _internal_format(internal_format), _data_format(data_format), _width(width),
        _height(height) {
    }

    static std::pair<GLenum, GLenum> format_from_channels(int channels) {
        if (channels == 4) {
            return { GL_RGBA8, GL_RGBA };
        }
        if (channels == 3) {
            return { GL_RGB8, GL_RGB };
        }
        fail_with_message("Format not supported");
    }

    GLuint _texture;
    GLenum _internal_format, _data_format;
    uint32_t _width, _height;
};

} // namespace cobalt