#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;
layout ( location = 1 ) in vec3 vertexNormal;

uniform mat4 modelTf = mat4( 1.0 );
uniform mat4 viewTf = mat4( 1.0 );
uniform mat4 projectionTf = mat4( 1.0 );
uniform mat4 inverseViewTf;
uniform mat4 inverseModelTf;

uniform vec3 light;

out vec3 normalWorld;
out vec3 lightWorld;
out vec4 positionWorld;

void main()
{
    positionWorld = modelTf * vec4( vertexPosition, 1.0 );
	
	normalWorld = normalize(inverseModelTf * vec4(vertexNormal,1)).xyz;
	
	lightWorld = light; 
	
	gl_Position = projectionTf * viewTf * modelTf * vec4( vertexPosition, 1.0 ); 
}