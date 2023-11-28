#version 450

struct VertexData {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

layout (set = 0, binding = 0) uniform GlobalUbo {
	mat4 proj_view;
} gubo;

layout (push_constant) uniform constants {
	mat4 model_mat;
	mat4 norm_mat;
} push;

layout (set = 1, binding = 0) readonly buffer Vertices {
	VertexData data[];
} vertices;

layout (set = 1, binding = 1) readonly buffer Indices {
	uint data[];
} indices;

#ifdef VERTEX_SHADER

void main() {
	//const array of positions for the triangle
	uint idx = indices.data[gl_VertexIndex];
	VertexData vtx = vertices.data[idx];
	vec4 world_space_pos = push.model_mat * vec4(vtx.x, vtx.y, vtx.z, 1.0f);

	gl_Position = gubo.proj_view * world_space_pos;
}

#endif
#ifdef FRAGMENT_SHADER
// no fragment shader
// cuz no color attachment
void main() {
}
#endif
