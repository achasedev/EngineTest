#version 450

// vec2 positions[3] = vec2[](
// 	vec2(0.0, -0.5),
// 	vec2(0.5, 0.5),
// 	vec2(-0.5, 0.5)
// );

// vec3 colors[3] = vec3[](
// 	vec3(1.0, 0.0, 0.0),
// 	vec3(0.0, 1.0, 0.0),
// 	vec3(0.0, 0.0, 1.0)
// );

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec4 COLOR;

layout(location = 0) out vec4 fragColor;

void main() 
{
	gl_Position = vec4(POSITION, 1.0);
	fragColor = COLOR;
}