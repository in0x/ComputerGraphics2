#version 330 core

out vec4 fragColor;

vec4 color = vec4( 1.0, 0.0, 0.0, 1.0 );

in vec3 normal;

void main()
{
	fragColor = vec4(normal, 1);
	//fragColor = color;
}