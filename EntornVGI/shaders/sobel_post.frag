#version 460 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform float uStrength = 1.5;

// NUEVO: color del borde (negro por defecto)
uniform vec3  uEdgeColor = vec3(0.0, 0.0, 0.0);
// NUEVO: multiplicador general de alpha (por si quieres "blend")
uniform float uAlphaMul  = 1.0;

float lum(vec3 c){ return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

void main(){
    ivec2 sz = textureSize(uScene, 0);
    vec2  texel = 1.0 / vec2(sz);

    float tl = lum(texture(uScene, vUV + texel*vec2(-1,-1)).rgb);
    float t  = lum(texture(uScene, vUV + texel*vec2( 0,-1)).rgb);
    float tr = lum(texture(uScene, vUV + texel*vec2( 1,-1)).rgb);
    float l  = lum(texture(uScene, vUV + texel*vec2(-1, 0)).rgb);
    float r  = lum(texture(uScene, vUV + texel*vec2( 1, 0)).rgb);
    float bl = lum(texture(uScene, vUV + texel*vec2(-1, 1)).rgb);
    float b  = lum(texture(uScene, vUV + texel*vec2( 0, 1)).rgb);
    float br = lum(texture(uScene, vUV + texel*vec2( 1, 1)).rgb);

    float gx = -tl - 2.0*l - bl + tr + 2.0*r + br;
    float gy = -tl - 2.0*t - tr + bl + 2.0*b + br;
    float mag = sqrt(gx*gx + gy*gy);

    float edge = clamp(mag * uStrength, 0.0, 1.0) * uAlphaMul;

    // Color configurable con alpha=edge
    FragColor = vec4(uEdgeColor, edge);
}
