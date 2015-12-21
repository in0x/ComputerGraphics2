#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;
layout ( location = 1 ) in vec3 vertexNormal;
layout ( location = 2 ) in vec2 vertexTexcoord;

out vec4 position;

uniform mat4 modelTf = mat4( 1.0 );
uniform mat4 viewTf = mat4( 1.0 );
uniform mat4 projectionTf = mat4( 1.0 );

void main()
{
	position = modelTf * vec4( vertexPosition, 1.0 );
	gl_Position = projectionTf * viewTf * position;
}