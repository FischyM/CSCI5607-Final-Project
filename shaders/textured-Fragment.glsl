#version 150 core

struct Material {
	float Ns;  // phong specularity exponent
	vec3 Ka;  // ambient light
	vec3 Kd;  // diffuse light
	vec3 Ks;  // specular light
  vec3 Ke;  // emissive light
};

#define POINT_LIGHTS_SIZE 9
in vec3 pointLightsPOS[POINT_LIGHTS_SIZE];
in vec3 pointLightsCOLOR[POINT_LIGHTS_SIZE];

in vec3 vertNormal;
in vec3 pos;
in vec2 texcoord;

uniform Material mat;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform int texID;

out vec4 outColor;


vec3 DiffuseAndSpecular(vec3 light_pos, vec3 light_col, Material mat, vec3 vertNormal, vec3 pos) {
  // reduce light intensity via distance
  float dist = length(light_pos-pos);
  vec3 I = light_col * (1.0/pow(dist,2));
  vec3 l = normalize(light_pos-pos);
  vec3 n = vertNormal;
  vec3 e = normalize(vec3(0,0,0)-pos);  //We know the eye is at (0,0)! (Do you know why?) 
  vec3 r = reflect(e,n);

  vec3 diffuseC = mat.Kd * I * max(dot(n,l), 0);

  float spec = max(dot(r,l),0.0);
  if (dot(-l,n) <= 0.0) spec = 0; //No highlight if we are not facing the light
  vec3 specC = mat.Ks * I * pow(spec, mat.Ns);

  vec3 emC = mat.Ke * I;

  vec3 oColor = diffuseC + specC + emC;
  return oColor;
}


void main() {
  vec3 ambientLight = vec3(0.7, 0.7, 0.7);
  vec3 oColor = vec3(0.0,0.0,0.0);;
  if (texID == -1) {
    oColor += mat.Ka * ambientLight;
    for (int i=0; i < POINT_LIGHTS_SIZE; i++) {
      oColor += DiffuseAndSpecular(pointLightsPOS[i], pointLightsCOLOR[i], mat, vertNormal, pos);
    }
    outColor = vec4(oColor, 1);
  }
  else if (texID == 0) {
    oColor = texture(tex0, texcoord).rgb* ambientLight;
    outColor = vec4(oColor,1);
  }
  else if (texID == 1) {
    oColor = texture(tex1, texcoord).rgb* ambientLight;
    outColor = vec4(oColor,1);
  }
  else {
    outColor = vec4(1,0,0,1);
  }
}
