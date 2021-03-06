/**
* @file Texture.h
*/
#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED
#include <GL/glew.h>
#include <memory>
#include <vector>

namespace Texture {

struct ImageData {
  GLint width;
  GLint height;
  GLenum format;
  GLenum type;
  std::vector<uint8_t> data;
};

GLuint CreateImage2D(GLsizei width, GLsizei height, const GLvoid* data, GLenum format, GLenum type);
GLuint LoadImage2D(const char* path);
bool LoadImage2D(const char* path, ImageData& imageData);

/**
* テクスチャ・イメージ.
*/
class Image2D
{
public:
  Image2D() = default;
  explicit Image2D(const char*);
  ~Image2D();
  bool IsNull() const;
  GLint Width() const { return width; }
  GLint Height() const { return height; }
  GLuint Id() const { return id; }
  GLenum Target() const;
  void Bind(int no) const;
  void Unbind(int no) const;

private:
  GLuint id = 0;
  GLint width = 0;
  GLint height = 0;
  bool isCubemap = false;
};
using Image2DPtr = std::shared_ptr<Image2D>;

} // namespace Texture

#endif // TEXTURE_H_INCLUDED