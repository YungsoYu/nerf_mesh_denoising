#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float FaceColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 boundaryColor;
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
    
    // Choose color based on face color flag
    // FaceColor: 0.0 = normal, 1.0 = boundary, 2.0 = green component, 3.0 = blue component
    vec3 color;
    if (FaceColor >= 3.0) {
        // Blue component (2nd largest)
        color = vec3(0.0, 0.0, 1.0);
    } else if (FaceColor >= 2.0) {
        // Green component (largest)
        color = vec3(0.0, 1.0, 0.0);
    } else if (FaceColor >= 1.0) {
        // Boundary face
        color = boundaryColor;
    } else {
        // Normal face
        color = objectColor;
    }
    
    vec3 result = (ambient + diffuse) * color;
    //vec3 result = color;
    FragColor = vec4(result, alpha);
}
