#version 330 core
// prop.fs — minimal lit-textured shader for outdoor props.
//
// Exists because core.fs combines its ambient and directional-light passes
// multiplicatively: `result = ambient * tinted; result *= CalcDirLight()`,
// where `CalcDirLight` itself contains another factor of `tinted`. That
// effectively squares the albedo for directional-lit scenes, which reads as
// "nearly black" on mid-value textures like tree bark and rock surfaces.
//
// Here we just do `lit = ambient * albedo + sun * ndl * albedo` — a single
// factor of albedo, ambient and sun are simple additive terms. A fragment
// with no sun contribution falls back cleanly to `ambient * albedo` instead
// of collapsing toward zero.
//
// The uniform names (`material.diffuse`, `material.color`, `material.opacity`,
// `ambient`) intentionally match core.fs so the existing
// `Object::render` / `Material::apply` binding paths set everything this
// shader needs without any special-casing on the C++ side.

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    vec3      diffuseColor;  // legacy tint, used when color.a <= 0.5
    vec4      color;         // rgb = tint; alpha>0.5 marks it as authoritative
    float     opacity;
    float     shininess;     // unused here, kept for uniform-shape compat
};

uniform Material material;

// Global ambient multiplier. Same uniform name core.fs uses, so
// `shader->setVec4("ambient", ...)` from demo code lands here too.
uniform vec4 ambient;

// Directional "sun". Set from the C++ side; defaults to the same values
// terrain.fs bakes in so a prop lit with this shader shades consistently
// with the terrain beneath it.
uniform vec3 sunDirection;
uniform vec3 sunColor;

void main() {
    vec3 n        = normalize(Normal);
    vec3 lightDir = normalize(sunDirection);
    float ndl     = max(dot(n, lightDir), 0.0);

    vec4 texColor = texture(material.diffuse, TexCoords);

    // Tint selection mirrors core.fs's convention so the same Material
    // wiring works for either shader.
    vec3 tint = (material.color.a > 0.5) ? material.color.rgb
                                         : material.diffuseColor;
    // Safety net: if both the color and the diffuseColor fallback are
    // unset (all zeros), treat it as "no tint" rather than multiplying
    // the texture by zero.
    if (dot(tint, tint) < 1e-6) tint = vec3(1.0);

    vec3 albedo = texColor.rgb * tint;

    // ambient * albedo  +  sun * ndl * albedo.
    // Both terms carry a single factor of albedo — no core.fs-style
    // double multiply that blackens mid-tones under a directional light.
    vec3 lit = ambient.rgb * albedo + sunColor * ndl * albedo;

    FragColor = vec4(lit, texColor.a * material.opacity);
}
