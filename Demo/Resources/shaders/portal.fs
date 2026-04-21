#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D portalTexture;

// Mirror overlay (Phase 1.5).
//
// When `hasMirrorOverlay` is true the sampled portal texture is tinted
// toward `mirrorTint` by `mirrorIntensity` ∈ [0, 1]:
//   intensity = 0  → exact destination view (no tint)
//   intensity = 1  → fully tinted (`base.rgb * mirrorTint`)
// `mirrorTint` defaults to (1, 1, 1) on the C++ side, which means even at
// intensity = 1 the result is unchanged unless the portal explicitly sets
// a coloured tint. Useful for sci-fi / cracked-glass mirrors.
uniform bool  hasMirrorOverlay;
uniform float mirrorIntensity;
uniform vec3  mirrorTint;

void main()
{
    vec4 base = texture(portalTexture, TexCoords);
    if (hasMirrorOverlay) {
        base.rgb = mix(base.rgb, base.rgb * mirrorTint, mirrorIntensity);
    }
    FragColor = base;
}
