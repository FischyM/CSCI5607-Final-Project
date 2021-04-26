#version 330

in vec3 position_in_eye_space;
in vec3 normal_in_eye_space;

out vec4 color;

uniform vec3 light_in_eye_space;
uniform vec4 Ia, Id, Is;

uniform vec4 ka, kd, ks;
uniform float s;


void main() {
    vec3 v = position_in_eye_space;
    vec3 n = normalize(normal_in_eye_space);

    // unit vector from the vertex to the light
    vec3 l = normalize(light_in_eye_space - v);
    
    // unit vector from the vertex to the eye point, which is at 0,0,0 in "eye space"
    vec3 e = normalize(vec3(0,0,0) - v);

    // halfway vector( halfway b/t l and e)
    vec3 h = normalize(l+e);

    // calculating lighting output the color for this vertex
    vec4 amb = ka * Ia;
    vec4 diff = kd * Id * max(dot(n, l), 0);
    vec4 spec = ks * Is * pow(max(dot(h, n), 0), s);

    vec4 colorFinal = amb + diff + spec;

    color = colorFinal;
}
