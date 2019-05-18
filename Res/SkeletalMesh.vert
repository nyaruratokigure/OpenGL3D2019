/**
* @file Sprite.vert
*/
#version 410

layout(location=0) in vec3 vPosition;
layout(location=1) in vec4 vColor;
layout(location=2) in vec2 vTexCoord;
layout(location=3) in vec3 vNormal;
layout(location=4) in vec4 vWeights;
layout(location=5) in vec4 vJoints;

layout(location=0) out vec4 outColor;
layout(location=1) out vec2 outTexCoord;
layout(location=2) out vec3 outNormal;
layout(location=3) out vec3 outPosition;

// global
uniform mat4x4 matVP;

// per mesh
layout(std140) uniform MeshMatrixUniformData
{
  vec4 color;
  mat3x4 matModel[4]; // it must transpose.
  mat3x4 matNormal[4]; // w isn't ussing. no need to transpose.
  mat3x4 matBones[256]; // it must transpose.
} vd;

// per primitive
uniform vec4 materialColor;
uniform int meshIndex;

/**
* �X�v���C�g�p���_�V�F�[�_�[.
*/
void main()
{
  outColor = vColor;
  outTexCoord = vTexCoord;
  if (dot(vWeights, vWeights) != 0.0) {
    mat3x4 matSkinTmp =
      vd.matBones[int(vJoints.x)] * vWeights.x +
      vd.matBones[int(vJoints.y)] * vWeights.y +
      vd.matBones[int(vJoints.z)] * vWeights.z +
      vd.matBones[int(vJoints.w)] * vWeights.w;
	mat4 matSkin = mat4(transpose(matSkinTmp));
	mat4 matModel = mat4(transpose(vd.matModel[meshIndex]));// * matSkin;
    outNormal = mat3(vd.matNormal[meshIndex]) * mat3(matSkin) * vNormal;
    outPosition = vec3(matModel * vec4(vPosition, 1.0));
    gl_Position = matVP * matModel * vec4(vPosition, 1.0);
  } else {
	mat4 matModel = mat4(transpose(vd.matModel[meshIndex]));
    outNormal = mat3(vd.matNormal[meshIndex]) * vNormal;
    outPosition = vec3(matModel * vec4(vPosition, 1.0));
    gl_Position = matVP * matModel * vec4(vPosition, 1.0);
  }
}