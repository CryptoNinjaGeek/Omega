#version 330 core
out vec4 FragColor;

in vec3 Normal;

void main()
{
    vec3 color = normalize(Normal);

    FragColor = vec4( 1.0, 1.0, 1.0, 1.0);
}
