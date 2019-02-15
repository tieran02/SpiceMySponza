#version 330

uniform mat4 projection_view_model_xform;
uniform mat4 projection_xform;
uniform mat4 view_xform;
uniform mat4 model_xform;

in vec3 vertex_position;
in vec3 vertex_normal;
in vec3 vertex_tangent;
in vec2 vertex_uv;

out vec3 vNormal;
out vec3 FragPos;
out vec2 UV;

void main(void)
{
	vNormal = mat3(model_xform) * vertex_normal;
	FragPos = mat4x3(model_xform) * vec4(vertex_position, 1.0);
	UV = vertex_uv;
	gl_Position = projection_view_model_xform * vec4(vertex_position, 1.0);
}
