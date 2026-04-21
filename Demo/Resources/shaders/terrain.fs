#version 330 core

// Terrain fragment shader — three-band height splat (low / mid / high) with
// directional sunlight. Each band is fed by a full Material (via the new
// self-binding Material::apply path), so any slot can have an optional
// normal / roughness / AO map and the shader branches on the per-slot
// `has*` flags rather than hardcoding per-unit samplers.
//
// Binding contract: the caller (Demo/Outdoor) calls
//     terrainObject_->setMaterials({sandMat, grassMat, rockMat})
// Object::render then invokes Material::apply under the prefixes
// "materials[0]" / "[1]" / "[2]" with firstUnit = 0, 5, 10 — matching
// Material::kApplyUnitCount per slot. Each apply() binds its own 5 texture
// units (base/normal/roughness/ao/metallic) and sets numeric uniforms.
//
// In practice the current outdoor scene uses:
//   materials[0]  — sand band  (baseColor only; normal/roughness/AO fall
//                    back to flat-normal / white via Material::apply)
//   materials[1]  — grass band (full PBR set: BaseColor + Normal +
//                    Roughness + AO)
//   materials[2]  — rock band  (baseColor only, same fallback story)
// Adding maps to sand/rock is now a one-liner on the Material in main.cpp;
// the shader automatically picks them up via the hasXxxMap flags.

out vec4 FragColor;

in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoords;
in float WorldHeight;

// Mirror of the apply()-side uniform contract. Keep field order and names
// in sync with Material::apply in Engine/src/render/Material.cpp — the
// binding side addresses these by "<prefix>.<name>" so a rename here must
// match a rename there.
struct MaterialSlot {
    sampler2D baseColor;
    sampler2D normalMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
    sampler2D metallicMap;
    int hasBaseColor;
    int hasNormalMap;
    int hasRoughnessMap;
    int hasAoMap;
    int hasMetallicMap;
    vec4  color;
    vec3  diffuse;
    float opacity;
    float shininess;
    vec2  uvTiling;
};

// Three splat bands: [0]=sand/beach, [1]=grass, [2]=rock/snow.
uniform MaterialSlot materials[3];

// Height bands in world units. Configurable so the same shader handles
// pocket islands (bands at [0, 3, 8]) and alpine terrain (bands at [0, 40,
// 80]) without recompile. The transition bands use smoothstep for soft
// blending; width controls how wide the blend is on each side.
uniform float bandLowMax  = 5.0;
uniform float bandMidMax  = 25.0;
uniform float bandWidth   = 2.5;

// Directional sun — see vs/fs comments upstream for convention details.
uniform vec3 sunDirection = vec3(-0.5, 0.75, -0.4);
uniform vec3 sunColor     = vec3(1.0, 0.95, 0.88);
uniform vec3 skyAmbient   = vec3(0.15, 0.17, 0.21);

// Base tint per terrain chunk (e.g. for debug LOD coloring later).
uniform vec3 tint = vec3(1.0, 1.0, 1.0);

// View position from Object::render, used for the specular half-vector.
uniform vec3 viewPos;

// Derive a tangent-space → world-space cotangent frame at the fragment
// from the geometric normal and the screen-space derivatives of world
// position and UV. Avoids needing per-vertex tangent/bitangent on the
// terrain mesh (terrain.vs only outputs position/normal/uv). Standard
// Mikkelsen-style derivation; slightly noisier than precomputed tangents
// but adequate for ground shading.
mat3 cotangentFrame(vec3 n, vec3 worldPos, vec2 uv) {
    vec3 dp1 = dFdx(worldPos);
    vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, n);
}

// One-band sample result, aggregated into the splat blend downstream.
struct BandSample {
    vec3  albedo;     // base colour × AO (AO=1 when slot has no AO map)
    vec3  normal;     // world-space shading normal (perturbed if slot has a
                      // normal map; geometric otherwise)
    float roughness;  // 1.0 when slot has no roughness map (→ no spec glare)
    float metallic;   // 0.0 when slot has no metallic map (dielectric)
    bool  hasNormal;  // forwarded so the specular branch can decide whether
                      // to rely on the perturbed normal for its half-vector
};

// We can't loop over materials[i] with a dynamic `i` because GLSL 330
// requires the sampler index in a struct array to be a constant expression.
// These three helpers inline the literal index, sample one slot of the
// splat, and resolve optional channels (hasXxxMap == 0) to neutral values
// so the downstream blend math doesn't have to re-check presence.
BandSample sampleBand0(vec3 nGeom, vec3 worldPos, vec2 baseUV) {
    vec2 uv = baseUV * materials[0].uvTiling;
    vec3 base = materials[0].hasBaseColor == 1
                ? texture(materials[0].baseColor, uv).rgb
                : vec3(1.0);
    float ao = materials[0].hasAoMap == 1
               ? texture(materials[0].aoMap, uv).r
               : 1.0;
    float rgh = materials[0].hasRoughnessMap == 1
                ? texture(materials[0].roughnessMap, uv).r
                : 1.0;
    float met = materials[0].hasMetallicMap == 1
                ? texture(materials[0].metallicMap, uv).r
                : 0.0;
    vec3 n = nGeom;
    if (materials[0].hasNormalMap == 1) {
        vec3 tsN = texture(materials[0].normalMap, uv).rgb * 2.0 - 1.0;
        n = normalize(cotangentFrame(nGeom, worldPos, uv) * tsN);
    }
    BandSample b;
    b.albedo = base * ao * materials[0].diffuse;
    b.normal = n;
    b.roughness = rgh;
    b.metallic = met;
    b.hasNormal = materials[0].hasNormalMap == 1;
    return b;
}
BandSample sampleBand1(vec3 nGeom, vec3 worldPos, vec2 baseUV) {
    vec2 uv = baseUV * materials[1].uvTiling;
    vec3 base = materials[1].hasBaseColor == 1
                ? texture(materials[1].baseColor, uv).rgb
                : vec3(1.0);
    float ao = materials[1].hasAoMap == 1
               ? texture(materials[1].aoMap, uv).r
               : 1.0;
    float rgh = materials[1].hasRoughnessMap == 1
                ? texture(materials[1].roughnessMap, uv).r
                : 1.0;
    float met = materials[1].hasMetallicMap == 1
                ? texture(materials[1].metallicMap, uv).r
                : 0.0;
    vec3 n = nGeom;
    if (materials[1].hasNormalMap == 1) {
        vec3 tsN = texture(materials[1].normalMap, uv).rgb * 2.0 - 1.0;
        n = normalize(cotangentFrame(nGeom, worldPos, uv) * tsN);
    }
    BandSample b;
    b.albedo = base * ao * materials[1].diffuse;
    b.normal = n;
    b.roughness = rgh;
    b.metallic = met;
    b.hasNormal = materials[1].hasNormalMap == 1;
    return b;
}
BandSample sampleBand2(vec3 nGeom, vec3 worldPos, vec2 baseUV) {
    vec2 uv = baseUV * materials[2].uvTiling;
    vec3 base = materials[2].hasBaseColor == 1
                ? texture(materials[2].baseColor, uv).rgb
                : vec3(1.0);
    float ao = materials[2].hasAoMap == 1
               ? texture(materials[2].aoMap, uv).r
               : 1.0;
    float rgh = materials[2].hasRoughnessMap == 1
                ? texture(materials[2].roughnessMap, uv).r
                : 1.0;
    float met = materials[2].hasMetallicMap == 1
                ? texture(materials[2].metallicMap, uv).r
                : 0.0;
    vec3 n = nGeom;
    if (materials[2].hasNormalMap == 1) {
        vec3 tsN = texture(materials[2].normalMap, uv).rgb * 2.0 - 1.0;
        n = normalize(cotangentFrame(nGeom, worldPos, uv) * tsN);
    }
    BandSample b;
    b.albedo = base * ao * materials[2].diffuse;
    b.normal = n;
    b.roughness = rgh;
    b.metallic = met;
    b.hasNormal = materials[2].hasNormalMap == 1;
    return b;
}

void main() {
    vec3 nGeom = normalize(Normal);

    // Three-band blend weights. smoothstep gives C1 transitions; we feed
    // the *raw* world height in, not a normalized [0,1] version, so the
    // bands stay meaningful even when the Heightmap has a large
    // verticalScale.
    float wLow  = 1.0 - smoothstep(bandLowMax - bandWidth, bandLowMax + bandWidth, WorldHeight);
    float wHigh = smoothstep(bandMidMax - bandWidth, bandMidMax + bandWidth, WorldHeight);
    float wMid  = max(0.0, 1.0 - wLow - wHigh);

    BandSample b0 = sampleBand0(nGeom, FragPos, TexCoords);
    BandSample b1 = sampleBand1(nGeom, FragPos, TexCoords);
    BandSample b2 = sampleBand2(nGeom, FragPos, TexCoords);

    // Blend albedo by band weight.
    vec3 albedo = b0.albedo * wLow + b1.albedo * wMid + b2.albedo * wHigh;
    albedo *= tint;

    // Blend the shading normal the same way, then re-normalize. Slots that
    // didn't perturb their normal already returned the geometric one, so
    // this degenerates to `nGeom` on a pure sand/rock patch.
    vec3 nShade = normalize(b0.normal * wLow + b1.normal * wMid +
                            b2.normal * wHigh);

    // Roughness blends linearly. Slots without a roughness map contributed
    // 1.0 (fully matte), which is the right fallback for sand/rock here.
    float roughness = b0.roughness * wLow + b1.roughness * wMid +
                      b2.roughness * wHigh;

    // Lambert + sky ambient on the blended albedo and shading normal.
    vec3 lightDir = normalize(sunDirection);
    float ndl = max(dot(nShade, lightDir), 0.0);
    vec3 lit  = skyAmbient * albedo + sunColor * ndl * albedo;

    // Crude spec — Blinn-Phong modulated by (1 - roughness). This is not
    // microfacet PBR; it's just enough so the roughness map visibly affects
    // the highlight. Replace when a proper BRDF pass lands.
    if (ndl > 0.0 && roughness < 0.999) {
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 halfDir = normalize(lightDir + viewDir);
        float specExponent = mix(8.0, 128.0, 1.0 - roughness);
        float specFactor   = pow(max(dot(nShade, halfDir), 0.0), specExponent);
        float specStrength = (1.0 - roughness) * 0.35;
        lit += sunColor * specFactor * specStrength;
    }

    FragColor = vec4(lit, 1.0);
}
