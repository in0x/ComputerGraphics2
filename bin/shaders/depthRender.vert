#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;

uniform mat4 projectionTf;
uniform mat4 viewTf;
uniform mat4 modelTf;

void main()
{
	gl_Position = projectionTf * viewTf * modelTf * vec4( vertexPosition, 1.0 );
}