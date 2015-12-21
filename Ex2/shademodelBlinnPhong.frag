#version 330 core

layout ( location = 0 ) out vec4 fragColor;

struct LightSource
{
    vec4 position_eyeSpace;
	vec4 position_worldSpace;
    vec3 color;
	float intensity;
};

const int MAX_LIGHTS = 10;
const float PI = 3.14159265359;

uniform LightSource lights[MAX_LIGHTS];
uniform mat4 modelViewTf;
uniform int lightCount;
uniform vec3 ambientColor;
uniform vec3 ambientIntensity;
uniform sampler2D diffuseTex;
uniform sampler2D specularTex;
uniform sampler2D aoTex;
uniform float shininess;

in vec4 position_eyeSpace;
in vec3 normal_eyeSpace;
in vec3 k_diffuse;
in vec2 uv;
in vec4 cameraPosition_eyeSpace;
in vec3 k_specular;

// brdfMicro = D * F * G / (4* dot(N,L)*dot(N,V))
// RadianceSpec = Irradiance * brdfMicro * kSpec * Norm
// D =  dot(H,N)^shininess
// F = fresnel
// G = 4 * dot(N,L) * dot(N,V)

float calcFschlick(in vec3 v0, in vec3 v1, in float f0)
{
	return f0 + (1.0 - f0) * pow((1.0 - dot(v0, v1)),5);
}

vec3 calcFschlick(in vec3 v0, in vec3 v1, in vec3 f0)
{
	return f0 + (1.0 - f0) * pow((1.0 - dot(v0, v1)),5);
}

void main()
{
	vec3 colorDiff = texture(diffuseTex, uv).rgb;
	vec3 colorSpec = texture(specularTex, uv).rgb;
	float ao = texture(aoTex, uv).r;
	
	vec3 R_sum = vec3(0.0, 0.0, 0.0);
	
	for (int i = 0; i < lightCount; i++) {
		float distance = length(position_eyeSpace.xyz - lights[i].position_eyeSpace.xyz);
		
		vec3 N = normalize(normal_eyeSpace);
		vec3 L = normalize(lights[i].position_eyeSpace.xyz - position_eyeSpace.xyz);
		vec3 V = normalize( cameraPosition_eyeSpace.xyz - position_eyeSpace.xyz );
		vec3 H = normalize( L + V );
		
		float dotNL = max( dot(N, L), 0.0);
	
		float n1 = 1.0;
		float n2 = 0.47;
		float f0 =((n1-n2)/(n1+n2))*((n1-n2)/(n1+n2));
		
		float E = lights[i].intensity * dotNL / (distance * distance);
		float fDiff = 1.0 - (f0+(1.0 - f0) * pow(dot(N,L),5)); 
		vec3 RadianceDiff = E * colorDiff * fDiff; //* (1 / PI);
		
		float D = pow(max(dot(H,N), 0.0),shininess);
	
		float F = f0 + (1.0 - f0) * pow((1.0 - (dot(L,N))),5);
		//float G = 4.0 * dot(N,L) * dot(N,V); weggekürzt
		float brdfMicro = D * F;
		vec3 RadianceSpec = E  * brdfMicro * colorSpec * ((shininess + 1.0) / (2.0 * PI));
		
		R_sum += ((RadianceSpec + max(RadianceDiff, 0.0)) * lights[i].color);
	}
	
	fragColor = vec4(ao * (R_sum + ambientColor * ambientIntensity ), 1.0); 
}

