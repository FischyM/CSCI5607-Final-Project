#version 150 core


in vec3 vertNormal;
in vec3 pos;
in vec2 texcoord;
in int matIndex;

struct Material {
	float Ns;  // phong specularity exponent
	vec3 Ka;  // ambient light
	vec3 Kd;  // diffuse light
	vec3 Ks;  // specular light
  vec3 Ke;  // emissive light
};

uniform Material mat;  // TODO: will need to remove this as well as in drawGeometry

Material mat_from_list;

#define MATERIAL_SIZE 13
uniform Material inMaterials[MATERIAL_SIZE];

out vec4 outColor;

uniform sampler2D tex0;
uniform sampler2D tex1;

uniform int texID;

#define POINT_LIGHTS_SIZE 9
in vec3 pointLightsDIR[POINT_LIGHTS_SIZE];
in vec3 pointLightsCOLOR[POINT_LIGHTS_SIZE];


vec3 DiffuseAndSpecular(vec3 lightDir, vec3 lightColor, Material material, vec3 vertNormal, vec3 pos) {
  // reduce light intensity via distance
  float dist = length(lightDir);
  vec3 normLightDir = normalize(lightDir);
  vec3 I = lightColor * (1.0/pow(dist,2));
  vec3 normal = normalize(vertNormal);
  vec3 diffuseC = material.Kd * I * max(dot(normal,normLightDir), 0.0);
  vec3 viewDir = normalize(-pos);  //We know the eye is at (0,0)! (Do you know why?) 
  vec3 reflectDir = reflect(viewDir,normal);
  float spec = max(dot(reflectDir,normLightDir), 0.0);
  //if (dot(-normLightDir,normal) <= 0.0) spec = 0; //No highlight if we are not facing the light
  vec3 specC = material.Ks * I * pow(spec, material.Ns);
  vec3 emC = material.Ke * I;
  vec3 oColor = diffuseC + specC + emC;
  return oColor;
}


void main() {
  vec3 ambientLight = vec3(0.3, 0.3, 0.3);
  mat_from_list = inMaterials[12];
  if (texID == -1) {
    vec3 oColor = vec3(0.0,0.0,0.0);
    oColor += mat_from_list.Ka * ambientLight;
    //point lights contributions
    for (int i=0; i < POINT_LIGHTS_SIZE; i++) {
      oColor += DiffuseAndSpecular(pointLightsDIR[i], pointLightsCOLOR[i], mat_from_list, vertNormal, pos);
    }
    outColor = vec4(oColor, 1);
  }
  else if (texID == 0) {
    vec3 oColor = texture(tex0, texcoord).rgb* ambientLight;
    outColor = vec4(oColor,1);
  }
  else if (texID == 1) {
    vec3 oColor = texture(tex1, texcoord).rgb* ambientLight;
    outColor = vec4(oColor,1);
  }
  else {
    outColor = vec4(1,0,0,1);
  }
}