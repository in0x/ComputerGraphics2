#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;
layout ( location = 1 ) in vec3 vertexNormal;
layout ( location = 2 ) in vec3 vertexTangent;
layout ( location = 3 ) in vec3 vertexBitangent;
layout ( location = 4 ) in vec2 vertexTexcoord;
layout ( location = 5 ) in vec3 vertexDiffuse;
layout ( location = 6 ) in vec3 vertexSpecular;

uniform mat4 modelviewTf;
uniform mat4 viewTf;
uniform mat4 projectionTf;
uniform mat3 normalTf_toEye;
uniform vec4 cameraPosition_worldSpace;	

out vec4 position_eyeSpace;
out vec3 normal_eyeSpace;
out vec3 k_diffuse;
out vec4 cameraPosition_eyeSpace;
out vec2 uv;
out vec3 k_specular;
out vec3 tangent_eyespace;
out vec3 bitangent_eyespace;

void main()
{
	uv = vertexTexcoord;
	
	position_eyeSpace = modelviewTf * vec4(vertexPosition, 1);
	normal_eyeSpace = normalTf_toEye * vertexNormal;
	tangent_eyespace = normalTf_toEye * vertexTangent;
	bitangent_eyespace = normalTf_toEye * vertexBitangent;
	cameraPosition_eyeSpace = viewTf * cameraPosition_worldSpace;
	k_diffuse = vertexDiffuse;
	k_specular = vertexSpecular;
	
	gl_Position = projectionTf * position_eyeSpace;
}
