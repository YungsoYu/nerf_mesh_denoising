#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float IsBoundary;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 boundaryColor;

void main()
{
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0);
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    // Choose color based on boundary flag
    vec3 color = mix(objectColor, boundaryColor, IsBoundary);
    
    vec3 result = (ambient + diffuse) * color;
    FragColor = vec4(result, 1.0);
}
