#version 150 core

in vec3 position;
in vec3 inNormal;
in vec2 inTexcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

#define POINT_LIGHTS_SIZE 9
uniform vec3 inPointLightsPOS[POINT_LIGHTS_SIZE];
uniform vec3 inPointLightsCOLOR[POINT_LIGHTS_SIZE];

out vec3 pointLightsDIR[POINT_LIGHTS_SIZE];
out vec3 pointLightsCOLOR[POINT_LIGHTS_SIZE];

out vec3 vertNormal;
out vec3 pos;
out vec2 texcoord;

void main() {
   gl_Position = proj * view * model * vec4(position,1.0);
   pos = (view * model * vec4(position,1.0)).xyz;

   for (int i=0; i < POINT_LIGHTS_SIZE; i++) {
      vec3 lightPos = (view * vec4(inPointLightsPOS[i],0.0)).xyz; //It's a vector!
      pointLightsDIR[i] = pos-lightPos;
      pointLightsCOLOR[i] = inPointLightsCOLOR[i];
   }
   // vec3 lightPos = (view * vec4(inLightPos,0.0)).xyz; //It's a vector!
   // lightDir = normalize(pos-lightPos);
   // lightColor = inLightColor;

   vec4 norm4 = transpose(inverse(view*model)) * vec4(inNormal,0.0);
   vertNormal = normalize(norm4.xyz);
   texcoord = inTexcoord;
}