#version 330 core

// Terrain fragment shader for Phase 1 MVP. Three-band height splat (low /
// mid / high) with directional sunlight. Intentionally minimal — Phase 4
// swaps this out for a five-band slope-aware splat with triplanar rock and
// detail noise, but keeping Phase 1 simple makes "walk-around test" fast to
// iterate on and gives the height bands a clean upgrade path.
//
// Textures are bound by the caller via `addTexture()`:
//   unit 0 — texLow  (sand/beach)
//   unit 1 — texMid  (grass)
//   unit 2 — texHigh (rock/snow)
// If a slot is missing, sample returns black; the app should supply all three.

out vec4 FragColor;

in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoords;
in float WorldHeight;

uniform sampler2D texLow;
uniform sampler2D texMid;
uniform sampler2D texHigh;

// Height bands in world units. Configurable so the same shader handles
// pocket islands (bands at [0, 3, 8]) and alpine terrain (bands at [0, 40,
// 80]) without recompile. The transition bands use smoothstep for soft
// blending; width controls how wide the blend is on each side.
uniform float bandLowMax  = 5.0;
uniform float bandMidMax  = 25.0;
uniform float bandWidth   = 2.5;

// Directional sun — single light for now. When Phase 6 brings cascaded
// shadow maps, this uniform becomes the primary sun direction and a separate
// shadow-sampling block gets added.
uniform vec3 sunDirection = vec3(-0.2, 1.0, -0.3);
uniform vec3 sunColor     = vec3(1.0, 0.95, 0.88);
uniform vec3 skyAmbient   = vec3(0.22, 0.26, 0.32);

// Base tint per terrain chunk (e.g. for debug LOD coloring later).
uniform vec3 tint = vec3(1.0, 1.0, 1.0);

void main() {
    vec3 n = normalize(Normal);

    // Three-band blend weights. smoothstep gives C1 transitions; we feed the
    // *raw* world height in, not a normalized [0,1] version, so the bands
    // stay meaningful even when the Heightmap has a large verticalScale.
    float wLow  = 1.0 - smoothstep(bandLowMax - bandWidth, bandLowMax + bandWidth, WorldHeight);
    float wHigh = smoothstep(bandMidMax - bandWidth, bandMidMax + bandWidth, WorldHeight);
    float wMid  = max(0.0, 1.0 - wLow - wHigh);

    vec3 cLow  = texture(texLow,  TexCoords).rgb;
    vec3 cMid  = texture(texMid,  TexCoords).rgb;
    vec3 cHigh = texture(texHigh, TexCoords).rgb;
    vec3 albedo = cLow * wLow + cMid * wMid + cHigh * wHigh;
    albedo *= tint;

    // Simple Lambert + ambient. sunDirection points *from* the sun *to* the
    // scene — match the convention DirectionalLight uses by negating for
    // lightDir.
    vec3 lightDir = normalize(-sunDirection);
    float ndl = max(dot(n, lightDir), 0.0);
    vec3 lit = skyAmbient * albedo + sunColor * ndl * albedo;

    FragColor = vec4(lit, 1.0);
}
