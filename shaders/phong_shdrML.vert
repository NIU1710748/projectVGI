#version 460 core
#define MaxLights 8

// -------- Estructuras 
struct Light {
    bool  sw_light;
    vec4  position;
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    vec3  attenuation;
    bool  restricted;
    vec3  spotDirection;
    float spotCosCutoff;
    float spotExponent;
};

struct Material {
    vec4  emission;
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    float shininess;
};

// -------- Atributs 
layout (location = 0) in vec3 in_Vertex;
layout (location = 1) in vec4 in_Color;
layout (location = 2) in vec3 in_Normal;
layout (location = 3) in vec2 in_TexCoord;

// -------- Uniforms 
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

// Aquí el projecte el fa servir com (view*model)^-T, ho respectem per seguir alimentant la pipeline antigua 
uniform mat4 normalMatrix;

uniform sampler2D texture0;
uniform bool  textur;
uniform bool  flag_invert_y;
uniform bool  fixedLight;
uniform bool  sw_material;
uniform bvec4 sw_intensity;
uniform vec4  LightModelAmbient;
uniform Light LightSource[MaxLights];
uniform Material material;

// -------- Sortidas (mantenim las del pipeline + noves per la llanterna)
out vec3 vertexPV;        // posició en espai de visita
out vec3 vertexNormalPV;  // normal en espai de vista
out vec2 vertexTexCoord;
out vec4 vertexColor;

out vec3 vWorldPos;       // Posició mon (per llinterna FPV)
out vec3 vNormalWS;       // Normal en mon (per linterna FPV)

void main()
{
    // --- Posicions ---
    vec4 worldPos = modelMatrix * vec4(in_Vertex, 1.0);
    vWorldPos     = worldPos.xyz;

    // --- Normals ---
    // 1) Normal en espai de vista, fent servir normalMatrix (view*model)^-T
    vertexNormalPV = normalize(mat3(normalMatrix) * in_Normal);

    // 2) Normal en espai de mon (per calcular amb cámara en mon)
    mat3 normalMatrixWS = transpose(inverse(mat3(modelMatrix)));
    vNormalWS = normalize(normalMatrixWS * in_Normal);

    // --- Posició en PV (vista) para tu pipeline antigua
    vertexPV = vec3(viewMatrix * worldPos);

    // --- Texcoords
    vertexTexCoord = flag_invert_y ? vec2(in_TexCoord.x, 1.0 - in_TexCoord.y)
                                   : in_TexCoord;

    // --- Color de vertex (si no fem servir material)
    vertexColor = in_Color;

    // --- Posició final de clip
    gl_Position = projectionMatrix * viewMatrix * worldPos;
}
