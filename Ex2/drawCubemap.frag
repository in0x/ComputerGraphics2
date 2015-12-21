#version 330 core

layout ( location = 0 ) out vec4 fragColor;

in vec4 position;

uniform samplerCube environmentTex;

void main()
{
	fragColor = texture( environmentTex, vec3( position.xy, -position.z ) );
}