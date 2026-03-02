#version 330 core

in vec3 vWorldPos;
in vec3 vWorldNormal;
in vec2 vUV;

out vec4 FragColor;

uniform vec3 uCamPos;
uniform vec3 uAlbedo;   // linear space
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;

uniform int uLightCount;
uniform vec3 uLightPos[4];
uniform vec3 uLightColor[4];

const float PI = 3.14159265358979323846;

// --- 1. Move helper functions OUTSIDE main() ---

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 1e-6);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 1e-6);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    float rough = clamp(uRoughness, 0.05, 1.0);

    // F0: Surface reflectance at zero incidence
    vec3 F0 = mix(vec3(0.04), uAlbedo, uMetallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; i++) { // Using 4 directly or uLightCount
        // Calculate per-light radiance
        vec3 L = normalize(uLightPos[i] - vWorldPos);
        vec3 H = normalize(V + L);
        float distance = length(uLightPos[i] - vWorldPos);
        float attenuation = 1.0 / max(distance * distance, 0.01);
        vec3 radiance = uLightColor[i] * attenuation;

        // Cook-Torrance BRDF
        float D = DistributionGGX(N, H, rough);
        float G = GeometrySmith(N, V, L, rough);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - uMetallic);

        vec3 numerator = D * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = numerator / max(denominator, 0.001);

        // Add to outgoing radiance
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * uAlbedo / PI + specular) * radiance * NdotL;
    }

    // --- 2. Ambient & Post-processing ---
    
    // Simple Ambient lighting
    vec3 F_amb = fresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kS_amb = F_amb;
    vec3 kD_amb = (vec3(1.0) - kS_amb) * (1.0 - uMetallic);
    
    vec3 ambientDiffuse = vec3(0.03) * uAlbedo * uAO * kD_amb;
    
    // Fake specular environment reflection
    float specStrength = pow(1.0 - rough, 2.0);
    vec3 ambientSpec = vec3(0.6, 0.7, 0.9) * kS_amb * 0.35 * specStrength * uAO;

    vec3 color = ambientDiffuse + ambientSpec + Lo;

    // Tonemapping & Gamma Correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}