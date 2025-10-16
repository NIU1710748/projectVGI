#version 460 core
#define MaxLights 8

// ---- Obra Dinn / Dither
uniform bool  uObraDinnOn;   // activa el modo
uniform float uThreshold;    // 0..1, p.ej. 0.5
uniform float uDitherAmp;    // fuerza del dither (0.2–0.5)
uniform bool  uGammaMap;     // mapear umbral a percepción (pow(x,2.2))
uniform bool uSobelMaskPass; //Para el sobel

float bayer8x8(vec2 p) {
    int x = int(mod(p.x, 8.0));
    int y = int(mod(p.y, 8.0));
    int M[64] = int[64](
         0,32, 8,40, 2,34,10,42,
        48,16,56,24,50,18,58,26,
        12,44, 4,36,14,46, 6,38,
        60,28,52,20,62,30,54,22,
         3,35,11,43, 1,33, 9,41,
        51,19,59,27,49,17,57,25,
        15,47, 7,39,13,45, 5,37,
        63,31,55,23,61,29,53,21
    );
    return float(M[y*8 + x]) / 64.0;
}


// ---- structs/material/lighting I/O (igual que tienes)
struct Light { bool sw_light; vec4 position; vec4 ambient; vec4 diffuse; vec4 specular;
               vec3 attenuation; bool restricted; vec3 spotDirection; float spotCosCutoff; float spotExponent; };
struct Material { vec4 emission; vec4 ambient; vec4 diffuse; vec4 specular; float shininess; };

in vec3 vertexPV;
in vec3 vertexNormalPV;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 normalMatrix, projectionMatrix, viewMatrix, modelMatrix;

uniform sampler2D texture0;
uniform bool textur, flag_invert_y, modulate, fixedLight, sw_material;
uniform bvec4 sw_intensity;
uniform vec4 LightModelAmbient;
uniform Light LightSource[MaxLights];
uniform Material material;

out vec4 FragColor;

// --- tu phongModel tal cual ---
vec3 phongModel(inout int numMat, inout float aValue) {
    vec3 N = normalize(vertexNormalPV);
    vec3 V = normalize(-vertexPV);
    float diffuseLight = 0.0;
    vec3 Idiffuse = vec3(0.0), Ispecular = vec3(0.0), ILlums = vec3(0.0), R = vec3(0.0);
    vec4 lightPosition = vec4(0.0,0.0,0.0,1.0);
    vec3 L = vec3(0.0), spotDirectionPV = vec3(0.0);
    float fatt = 1.0, spotDot;

    vec3 Iemissive = vec3(0.0);
    if (sw_intensity[0]) {
        if (sw_material) { Iemissive = material.emission.rgb; numMat++; aValue = material.emission.a; }
        else Iemissive = vertexColor.rgb;
        Iemissive = clamp(Iemissive, 0.0, 1.0);
    }

    vec3 Iambient = vec3(0.0);
    if (sw_intensity[1]) {
        if (sw_material) { Iambient = material.ambient.rgb * LightModelAmbient.rgb; numMat++; aValue = material.ambient.a; }
        else Iambient = vertexColor.rgb * LightModelAmbient.rgb;
        Iambient = clamp(Iambient, 0.0, 1.0);
    }

    for (int i=0;i<MaxLights;i++) if (LightSource[i].sw_light) {
        fatt = 1.0;
        lightPosition = fixedLight ? (viewMatrix * LightSource[i].position) : LightSource[i].position;
        if (LightSource[i].position.w == 1.0) {
            L = lightPosition.xyz - vertexPV;
            float d = length(L);
            fatt = 1.0 / (LightSource[i].attenuation.x*d*d + LightSource[i].attenuation.y*d + LightSource[i].attenuation.z);
            L = normalize(L);
        } else {
            L = normalize(-lightPosition.xyz);
        }

        if (LightSource[i].restricted) {
            spotDirectionPV = vec3(normalMatrix * vec4(LightSource[i].spotDirection,1.0));
            spotDirectionPV = normalize(spotDirectionPV);
            spotDot = dot(-L, spotDirectionPV);
            if (spotDot > LightSource[i].spotCosCutoff) fatt *= pow(spotDot, LightSource[i].spotExponent);
            else fatt = 0.0;
        }

        if (sw_intensity[2]) {
            diffuseLight = max(dot(L, N), 0.0);
            if (sw_material) { Idiffuse = material.diffuse.rgb * LightSource[i].diffuse.rgb * diffuseLight * fatt; numMat++; aValue += material.diffuse.a; }
            else Idiffuse = vertexColor.rgb * LightSource[i].diffuse.rgb * diffuseLight * fatt;
            Idiffuse = clamp(Idiffuse, 0.0, 1.0);
        }

        if (sw_intensity[3]) {
            R = normalize(-reflect(L,N));
            float specularLight = pow(max(dot(R, V), 0.0), material.shininess);
            if (sw_material) { Ispecular = material.specular.rgb * LightSource[i].specular.rgb * specularLight * fatt; numMat++; aValue += material.specular.a; }
            else Ispecular = vertexColor.rgb * LightSource[i].specular.rgb * specularLight * fatt;
            Ispecular = clamp(Ispecular, 0.0, 1.0);
        }

        ILlums += Idiffuse + Ispecular;
    }

    return Iemissive + Iambient + ILlums;
}

void main() {

    if (uSobelMaskPass) {
        // Pasada de máscara: objeto = blanco liso, fondo ya es negro por clear
        FragColor = vec4(1.0);
        return;
    }

    int   numMaterials = 0;
    float aValor       = 0.0;

    vec4 lit = vec4(0.0, 0.0, 0.0, 1.0);
    lit.rgb = phongModel(numMaterials, aValor);
    lit.a   = sw_material ? ((numMaterials != 0) ? (aValor / float(numMaterials)) : 1.0)
                          : vertexColor.a;

    // Mezcla con textura si existe
    vec4 base = lit;
    if (textur) {
        vec4 tex = texture(texture0, vertexTexCoord);
        base = modulate ? (tex * lit) : tex;
        base.a = tex.a * lit.a;
    }

    if (uObraDinnOn) {
        float luma = dot(base.rgb, vec3(0.2126, 0.7152, 0.0722));
        float thr  = uGammaMap ? pow(uThreshold, 2.2) : uThreshold;
        float d    = bayer8x8(gl_FragCoord.xy) - 0.5; // [-0.5, 0.5]
        thr = clamp(thr + d * uDitherAmp, 0.0, 1.0);
        float bw = (luma > thr) ? 1.0 : 0.0;
        FragColor = vec4(vec3(bw), base.a);
        return;
    }

    FragColor = base;
}
