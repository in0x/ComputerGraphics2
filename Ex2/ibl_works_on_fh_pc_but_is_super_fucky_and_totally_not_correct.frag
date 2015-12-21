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
uniform float roughness;
uniform float anisotropy;

in vec4 position_eyeSpace;
in vec3 normal_eyeSpace;
in vec3 k_diffuse;
in vec2 uv;
in vec4 cameraPosition_eyeSpace;
in vec3 k_specular;
in vec3 tangent_eyespace;
in vec3 bitangent_eyespace;

void main()
{
	vec3 colorDiff = texture(diffuseTex, uv).rgb;
	vec3 colorSpec = texture(specularTex, uv).rgb;
	float ao = texture(aoTex, uv).r;
	//vec3 colorDiff = k_diffuse;
	//vec3 colorSpec = k_specular;
	//float ao = 1; 
	
	
	float alphaX = roughness * roughness;
	float alphaY = (2.0 - 2.0 * anisotropy) * alphaX;
	
	vec3 T = normalize(tangent_eyespace);
	vec3 B = normalize(bitangent_eyespace);
	vec3 N = normalize(normal_eyeSpace);
	vec3 V = normalize( cameraPosition_eyeSpace.xyz - position_eyeSpace.xyz );
	
	vec3 R_sum = vec3(0.0, 0.0, 0.0);
	
	for (int i = 0; i < lightCount; i++) {
		float distance = length(position_eyeSpace.xyz - lights[i].position_eyeSpace.xyz);
		
		vec3 L = normalize(lights[i].position_eyeSpace.xyz - position_eyeSpace.xyz);
		vec3 H = normalize( L + V );
		
		float dotNL = max( dot(N, L), 0.0);
	
		float n1 = 1.0;
		float n2 = 0.47;
		float f0 =((n1-n2)/(n1+n2))*((n1-n2)/(n1+n2));
		
		float E = lights[i].intensity * dotNL / (distance * distance);
		float fDiff = 1.0 - (f0+(1.0 - f0) * pow(dot(N,L),5)); 
		vec3 RadianceDiff = E * colorDiff * fDiff; //* (1 / PI);
		
		//float D = pow((pow(dot(T,H),2) / pow(alphaX,2) + pow(dot(B, H),2) / pow(alphaY,2) + pow(dot(N,H),2)), -2);
		
		float D = 1 /pow(max(pow(dot(T,H),2),0.0) / pow(alphaX,2) + max(pow(dot(B, H),2),0.0) / pow(alphaY,2) + max(pow(dot(N,H),2), 0.0), 2);
		
		float k = (roughness*roughness) /4;
		float GL = max(dot(N,L), 0.0) / (max(dot(N,L),0.0) * (1 - k)) + k;
		float GV = max(dot(N,V),0.0) / (max(dot(N,V),0.0) * (1 - k)) + k; //could be source of error ()
		float G = GL * GV;
		
		float F = f0 + (1.0 - f0) * pow((1.0 - (dot(L,N))),5);
		//float G = 4.0 * max(dot(N,L),0.0) * max(dot(N,V),0.0); weggekürzt
		float brdfMicro = D * F * G / (1 / (PI*alphaX*alphaY));
		vec3 RadianceSpec = E  * brdfMicro * colorSpec * (4*max(dot(N,V),0.0)*max(dot(N,L),0.0));
		
		R_sum += ((max(RadianceSpec, 0.0) + max(RadianceDiff, 0.0)) * lights[i].color);
		//R_sum += D;
	}
	
	fragColor = vec4(ao * (R_sum + (ambientColor * ambientIntensity )), 1.0); 
	
}
