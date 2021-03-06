/**
* @file Mesh.h
*/
#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED
#include <GL/glew.h>
#include "BufferObject.h"
#include "json11/json11.hpp"
#include "Texture.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace Mesh {

class Buffer;
using BufferPtr = std::shared_ptr<Buffer>;
struct Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

// 頂点データ.
struct Vertex
{
  glm::vec3 position;
  glm::vec4 color;
  glm::vec2 texCoord;
  glm::vec3 normal;
  GLfloat   weights[4];
  GLushort  joints[4];
};

// 頂点属性.
struct VertexAttribute
{
  GLuint index = GL_INVALID_INDEX;
  GLint size = 0;
  GLenum type = GL_FLOAT;
  GLsizei stride = 0;
  size_t offset = 0;
};

// マテリアル.
struct Material {
  glm::vec4 baseColor = glm::vec4(1);
  Texture::Image2DPtr texture;
};

// メッシュプリミティブ.
struct Primitive {
  GLenum mode;
  GLsizei count;
  GLenum type;
  const GLvoid* indices;
  GLint baseVertex = 0;
  VertexAttribute attributes[8];
  int material;
};

// メッシュデータ.
struct MeshData {
  std::string name;
  std::vector<Primitive> primitives;
};

// スキンデータ.
struct Skin {
  std::string name;
  std::vector<int> joints;
};

// ノード.
struct Node {
  Node* parent = nullptr;
  int mesh = -1;
  int skin = -1;
  std::vector<Node*> children;
  glm::mat4 matLocal = glm::mat4(1);
  glm::mat4 matGlobal = glm::mat4(1);
  glm::mat4 matInverseBindPose = glm::mat4(1);
};

// アニメーションのキーフレーム.
template<typename T>
struct KeyFrame {
  float frame;
  T value;
};

// アニメーションのタイムライン.
template<typename T>
struct Timeline {
  int targetNodeId;
  std::vector<KeyFrame<T>> timeline;
};

// アニメーション.
struct Animation {
  std::vector<Timeline<glm::vec3>> translationList;
  std::vector<Timeline<glm::quat>> rotationList;
  std::vector<Timeline<glm::vec3>> scaleList;
  float totalTime = 0;
};

// シーン.
struct Scene {
  int rootNode;
  std::vector<const Node*> meshNodes;
};


// ファイル.
struct File {
  std::string name; // ファイル名.
  std::vector<Scene> scenes;
  std::vector<Node> nodes;
  std::vector<MeshData> meshes;
  std::vector<Material> materials;
  std::vector<Skin> skins;
  std::vector<Animation> animations;
  const VertexArrayObject* vao = nullptr;
};
using FilePtr = std::shared_ptr<File>;

struct MeshTransformation {
  std::vector<glm::mat4> transformations;
  std::vector<glm::mat4> matRoot;
};

/**
* 描画するメッシュデータ.
*
* Primitveをまとめたもの.
* シーンルートノードからメッシュノードまでの変換行列を掛けないと、正しい姿勢で描画されない場合がある.
* アニメーションを持つのがボーンノードだけなら、これは一度計算すれば十分.
* だが、通常ノードがアニメーションを持っている場合もあるので、その場合は毎回ルートノードから再計算する必要がある.
* glTFのアニメーションデータには通常ノードとボーンノードの区別がないので、区別したければアニメーションデータの対象となるノードが
* スケルトンノードリストに含まれるかどうかで判定する必要がありそう.
*
* 作成方法:
* - Buffer::GetMeshで作成.
*
* 更新方法:
* -# Buffer::ResetUniformData()でUBOバッファをリセット.
* -# 描画するすべてのメッシュについて、Mesh::UpdateでアニメーションとUBOバッファの更新(これは分けたほうがいいかも).
* -# Buffer::UploadUniformData()でUBOバッファをVRAMに転送.
*
* 描画方法:
* - Mesh::Drawで描画.
*/
struct Mesh
{
  Mesh() = default;
  Mesh(Buffer* p, const FilePtr& f, const Node* n) : parent(p), file(f), node(n) {}
  void Draw() const;
  void Update(float deltaTime);
  void SetAnimation(int);
  int GetAnimation() const;
  size_t GetAnimationCount() const;
  MeshTransformation CalculateTransform() const;

  std::string name;

  GLenum mode = GL_TRIANGLES;
  GLsizei count = 0;
  GLenum type = GL_UNSIGNED_SHORT;
  const GLvoid* indices = 0;
  GLint baseVertex = 0;
  
  Buffer* parent = nullptr;

  FilePtr file;
  const Node* node = nullptr;
  const Animation* animation = nullptr;

  glm::vec3 translation = glm::vec3(0);
  glm::quat rotation = glm::quat(glm::vec3(0));
  glm::vec3 scale = glm::vec3(1);
  glm::vec4 color = glm::vec4(1);

  float frame = 0;

  Shader::ProgramPtr shader;
  GLintptr uboOffset = 0;
  GLsizeiptr uboSize = 0;
};

/**
* メッシュ描画用UBOデータ.
*/
struct alignas(256) UniformDataMeshMatrix
{
  glm::vec4 color;
  glm::mat3x4 matModel[4]; // it must transpose.
  glm::mat3x4 matNormal[4]; // w isn't ussing. no need to transpose.
  glm::mat3x4 matBones[256]; // it must transpose.
};

/**
* 全ての描画データを保持するクラス.
*
* メッシュの登録方法:
* - 頂点データとインデックスデータを用意する.
* - Meshオブジェクトを用意し、メッシュ名を決めてMesh::nameに設定する.
* - AddVertexData()で頂点の座標、テクスチャ座標、色などを設定し、戻り値をVertexサイズで割った値をbaseVertexに設定する.
* - AddIndexData()でインデックスデータを設定し、戻り値をMesh::indicesに設定する.
* - Meshのmode, count, typeメンバーに適切な値を設定する.
*   たとえば座標を頂点アトリビュートの0番目として指定する場合、Mesh::attributes[0]::offsetに設定する.
* - 頂点アトリビュートのindex, size, type, strideに適切な値を設定する.
* - AddMesh()でメッシュを登録する.
*/
class Buffer
{
public:
  Buffer() = default;
  ~Buffer() = default;

  bool Init(GLsizeiptr vboSize, GLsizeiptr iboSize, GLsizeiptr uboSize);
  GLintptr AddVertexData(const void* data, size_t size);
  GLintptr AddIndexData(const void* data, size_t size);
  void AddMesh(const Mesh&);
  bool LoadMesh(const char* path);
  MeshPtr GetMesh(const char* meshName) const;
  void Bind();
  void Unbind();

  void CreateCube(const char* name);
  void CreateCircle(const char* name, size_t segments);
  void CreateSphere(const char* name, size_t segments, size_t rings);

  void ResetUniformData();
  GLintptr PushUniformData(const void* data, size_t size);
  void UploadUniformData();
  void BindUniformData(GLintptr offset, GLsizeiptr size);

private:
  BufferObject vbo;
  BufferObject ibo;
  VertexArrayObject vao;
  GLintptr vboEnd = 0;
  GLintptr iboEnd = 0;
  struct MeshIndex {
    FilePtr file;
    const Node* node = nullptr;
  };
  std::unordered_map<std::string, MeshIndex> meshes;
  std::unordered_map<std::string, FilePtr> files;

  UniformBufferPtr ubo;
  std::vector<uint8_t> uboData;
\
  bool SetAttribute(
    Primitive& prim, const json11::Json& accessor, const json11::Json& bufferViews, std::vector<std::vector<char>>& binFiles, int index, int size);
};

} // namespace Mesh

#endif // MESH_H_INCLUDED