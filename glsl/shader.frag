#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float FaceColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float alpha;

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

    vec3 color;
    //int faceColorInt = int(FaceColor);
    if (FaceColor == 1.0) {
        color = vec3(1.0, 1.0, 0.0);  // Yellow
    } else if (FaceColor == 2.0) {
        color = vec3(1.0, 0.0, 0.0);  // Red
    } else if (FaceColor == 3.0) {
        color = vec3(1.0, 0.5, 0.0);  // Orange
    } else {
        color = vec3(0.9, 0.9, 0.9);  // Default case
    }
    
    vec3 result = (ambient + diffuse) * color;
    //vec3 result = color;
    FragColor = vec4(result, alpha);
}
