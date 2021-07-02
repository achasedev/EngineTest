#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} cameraUBO;

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec4 COLOR;

layout(location = 0) out vec4 fragColor;

void main() 
{
	gl_Position = cameraUBO.projection * cameraUBO.view * cameraUBO.model * vec4(POSITION, 1.0);
	fragColor = COLOR;
}