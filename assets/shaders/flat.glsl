#version 450
//#extension GL_EXT_debug_printf : enable

#ifdef VERTEX_SHADER

struct VertexData {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

layout(set = 0, binding = 0) uniform  GlobalUbo {
	mat4 proj;
	mat4 view;
	mat4 proj_view;
} gubo;

layout(set = 1, binding = 0) uniform  ObjectUbo {
	vec4 color;
} oubo;

layout (set = 2, binding = 0) readonly buffer Vertices {
	VertexData data[];
} vertices;

layout (set = 2, binding = 1) readonly buffer Indices {
	uint data[];
} indices;

layout (push_constant) uniform constants {
	mat4 model_mat;
} push;

layout (location = 0) out vec3 o_norm;

// vertex shader
void main() {
	//const array of positions for the triangle
	uint idx = indices.data[gl_VertexIndex];
	VertexData vtx = vertices.data[idx];
	vec3 pos = vec3(vtx.x, vtx.y, vtx.z);


	//output the position of each vertex
	o_norm = vec3(vtx.nx, vtx.ny, vtx.nz);
	gl_Position = gubo.proj_view * push.model_mat * vec4(pos, 1.0f);
}

#endif
#ifdef FRAGMENT_SHADER

layout (location = 0) in vec3 i_norm;
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform  GlobalUbo {
	mat4 proj;
	mat4 view;
	mat4 proj_view;
} gubo;

layout(set = 1, binding = 0) uniform  ObjectUbo {
	vec4 color;
} oubo;

layout (push_constant) uniform constants {
	vec4 model_mat;
} PushConstants;


// fragment shader
void main() {
//	outFragColor = vec4(i_norm, 1.0);
	outFragColor = oubo.color;
}

#endif
