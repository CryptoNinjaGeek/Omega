#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    vec4 color;
    float shininess;
};

in vec2 TexCoords;
in vec3 Normal;
uniform Material material;

void main()
{
    vec4 texture_color = vec4(texture(material.diffuse, TexCoords));

    FragColor = vec4(material.color.x, material.color.y, material.color.z, texture_color.a);
}
