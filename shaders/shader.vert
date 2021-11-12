#version 450
#extension GL_EXT_debug_printf : enable
#define M_PI 3.1415926535897932384626433832795
layout(location = 0) in int vertexCount;
layout(location = 0) out vec3 fragColor;

void main() {
    const float radius = 0.3;
    float currentAngle = (gl_VertexIndex / float(vertexCount)) * 2 * M_PI;
    debugPrintfEXT("currentAngle is %f", currentAngle);
    float YOffset = radius * sin(currentAngle);
    float XOffset = radius * cos(currentAngle);
    gl_Position = vec4(XOffset, -YOffset, 0, 1);
    fragColor = vec3(1.0, 0.0, 0.0);
}