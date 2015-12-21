#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;
layout ( location = 1 ) in vec3 vertexNormal;
layout ( location = 2 ) in vec3 vertexColor;

uniform mat4 projectionTf;
uniform mat4 viewTf;
uniform mat4 modelTf;

out vec3 color;
out vec3 normal_eyeSpace;
out vec4 position_eyeSpace;

void main()
{
	color = vertexColor;
	mat4 modelviewTf = viewTf * modelTf;
	mat3 normalTf = inverse( transpose( mat3( modelviewTf ) ) );
	normal_eyeSpace = normalize( normalTf * vertexNormal );
	position_eyeSpace = modelviewTf * vec4( vertexPosition, 1.0 );
	gl_Position = projectionTf * position_eyeSpace;
}