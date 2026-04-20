#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
    vec4 color;        // Material color (RGBA) - tints the texture
    vec3 diffuseColor;  // Material diffuse color (RGB) - alternative tint
    float opacity;     // Material opacity (0.0-1.0)
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    int on;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    int on;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    int on;
};

#define NR_POINT_LIGHTS 4
#define NR_SPOT_LIGHTS 4
#define NR_DIR_LIGHTS 4

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform DirLight dirLight[NR_DIR_LIGHTS];
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight[NR_SPOT_LIGHTS];
uniform Material material;
uniform vec4 ambient;


// function prototypes
vec4 CalcAmbientLight();
vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // == =====================================================
    // Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
    // For each phase, a calculate function is defined that calculates the corresponding color
    // per lamp. In the main() function we take all the calculated colors and sum them up for
    // this fragment's final color.
    // == =====================================================
    // phase 1: directional lighting
    vec4 result = CalcAmbientLight();

    for(int i = 0; i < NR_DIR_LIGHTS; i++){
        if(dirLight[i].on == 1){
            result *= CalcDirLight(dirLight[i], norm, viewDir);
        }
    }

    // phase 2: point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(pointLights[i].on == 1){
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
        }
    }

    // phase 3: spot light
    for(int i = 0; i < NR_SPOT_LIGHTS; i++){
        if(spotLight[i].on == 1){
            result += CalcSpotLight(spotLight[i], norm, FragPos, viewDir);
        }
    }

    // Apply material opacity to final color
    FragColor = vec4(result.rgb, result.a * material.opacity);
}

// calculates the color when using a directional light.
vec4 CalcAmbientLight()
{
    vec4 texColor = texture(material.diffuse, TexCoords);
    // Apply material color/diffuse tint if available
    // Use color if alpha > 0.5 (explicitly set), otherwise use diffuseColor
    vec3 materialTint = (material.color.a > 0.5) ? material.color.rgb : material.diffuseColor;
    // Blend texture with material color (multiply for tinting effect)
    vec4 tintedColor = vec4(texColor.rgb * materialTint, texColor.a);
    return ambient * tintedColor;
}

// calculates the color when using a directional light.
vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec4 texColor = texture(material.diffuse, TexCoords);
    // Apply material color/diffuse tint
    // Use color if alpha > 0.5 (explicitly set), otherwise use diffuseColor
    vec3 materialTint = (material.color.a > 0.5) ? material.color.rgb : material.diffuseColor;
    vec4 tintedColor = vec4(texColor.rgb * materialTint, texColor.a);
    vec4 ambientColor = vec4(light.ambient, 1.0) * tintedColor;
    vec4 diffuseColor = vec4(light.diffuse, 1.0) * diff * tintedColor;
//    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambientColor + diffuseColor);// + specular);
}

// calculates the color when using a point light.
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec4 texColor = texture(material.diffuse, TexCoords);
    vec4 specTexColor = texture(material.specular, TexCoords);
    // Apply material color/diffuse tint
    vec3 materialTint = (material.color.a > 0.0) ? material.color.rgb : material.diffuseColor;
    vec4 tintedColor = vec4(texColor.rgb * materialTint, texColor.a);
    vec4 ambientColor = vec4(light.ambient, 1.0) * tintedColor;
    vec4 diffuseColor = vec4(light.diffuse, 1.0) * diff * tintedColor;
    vec4 specularColor = vec4(light.specular, 1.0) * spec * specTexColor;
    ambientColor *= attenuation;
    diffuseColor *= attenuation;
    specularColor *= attenuation;
    return (ambientColor + diffuseColor + specularColor);
}

// calculates the color when using a spot light.
vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec4 texColor = texture(material.diffuse, TexCoords);
    vec4 specTexColor = texture(material.specular, TexCoords);
    // Apply material color/diffuse tint
    vec3 materialTint = (material.color.a > 0.0) ? material.color.rgb : material.diffuseColor;
    vec4 tintedColor = vec4(texColor.rgb * materialTint, texColor.a);
    vec4 ambientColor = vec4(light.ambient, 1.0) * tintedColor;
    vec4 diffuseColor = vec4(light.diffuse, 1.0) * diff * tintedColor;
    vec4 specularColor = vec4(light.specular, 1.0) * spec * specTexColor;
    ambientColor *= attenuation * intensity;
    diffuseColor *= attenuation * intensity;
    specularColor *= attenuation * intensity;
    return (ambientColor + diffuseColor + specularColor);
}