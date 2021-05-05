#version 150 core

#define POINT_LIGHTS_SIZE 16
in vec3 pointLightsPOS[POINT_LIGHTS_SIZE];
//in vec3 pointLightsCOLOR[POINT_LIGHTS_SIZE];
uniform vec3 inPointLightsCOLOR[POINT_LIGHTS_SIZE];

in vec3 vertNormal;
in vec3 pos;
in vec2 texcoord;
in float matIndex;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

uniform int texID;

#define MATERIAL_SIZE 24
uniform vec3 inMatsKa[MATERIAL_SIZE];
uniform vec3 inMatsKs[MATERIAL_SIZE];
uniform vec3 inMatsKd[MATERIAL_SIZE];
uniform vec3 inMatsKe[MATERIAL_SIZE];
uniform float inMatsNs[MATERIAL_SIZE];
struct Material {
	float Ns;  // phong specularity exponent
	vec3 Ka;  // ambient light
	vec3 Kd;  // diffuse light
	vec3 Ks;  // specular light
  vec3 Ke;  // emissive light
};

uniform bool useMat;
uniform Material mat;  // TODO: will need to remove this as well as in drawGeometry, used as a single material property for object

out vec4 outColor;

vec3 DiffuseAndSpecular(vec3 light_pos, vec3 light_col, vec3 mat_Kd, vec3 mat_Ks, float mat_Ns, vec3 vertNormal, vec3 pos) {
  // reduce light intensity via distance
  float dist = length(light_pos-pos);
  vec3 I = light_col * (1.0/pow(dist,2));
  vec3 l = normalize(light_pos-pos);
  vec3 n = vertNormal;
  vec3 e = normalize(vec3(0,0,0)-pos);  //We know the eye is at (0,0)! (Do you know why?) 
  vec3 r = reflect(e,n);

  vec3 diffuseC = mat_Kd * I * max(dot(n,l), 0);

  float spec = max(dot(r,l),0.0);
  if (dot(-l,n) <= 0.0) spec = 0; //No highlight if we are not facing the light
  vec3 specC = mat_Ks * I * pow(spec, mat_Ns);

  vec3 oColor = diffuseC + specC;
  return oColor;
}


void main() {
  vec3 ambientLight = vec3(0.001);
  vec3 textAmbLight = vec3(0.05);
  vec3 oColor = vec3(0.0,0.0,0.0);
  int matInd = int(round(matIndex));
  Material vert_mat;
  if (useMat) {
    vert_mat = mat;
  }
  else {
    vert_mat.Ka=inMatsKa[matInd]; vert_mat.Kd=inMatsKd[matInd]; vert_mat.Ks=inMatsKs[matInd]; vert_mat.Ns=inMatsNs[matInd]; vert_mat.Ke=inMatsKe[matInd];
  }
  if (texID == -1) {
    oColor += vert_mat.Ka * ambientLight;
    for (int i=0; i < POINT_LIGHTS_SIZE; i++) {
      oColor += DiffuseAndSpecular(pointLightsPOS[i], inPointLightsCOLOR[i], vert_mat.Kd, vert_mat.Ks, vert_mat.Ns, vertNormal, pos);
    }
    oColor += vert_mat.Ke;
    oColor = clamp(oColor, 0.0, 1.0);
    outColor = vec4(oColor, 1);
  }
  else if (texID == 0) {
    oColor = texture(tex0, texcoord).rgb * textAmbLight;
    outColor = vec4(oColor,1);
  }
  else if (texID == 1) {
    oColor = texture(tex1, texcoord).rgb * textAmbLight;
    outColor = vec4(oColor,1);
  }
  else if (texID == 2) {
    oColor = texture(tex2, texcoord).rgb * textAmbLight;
    outColor = vec4(oColor,1);
  }
  else {
    outColor = vec4(1,0,0,1);
  }
}
