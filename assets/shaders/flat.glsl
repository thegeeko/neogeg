#version 450
//#extension GL_EXT_debug_printf : enable

#ifdef VERTEX_SHADER

struct VertexData {
  float x, y, z;
  float nx, ny, nz;
  float u, v;
};

layout(binding=0) readonly buffer Vertices { 
	VertexData data[]; 
} in_Vertices;

layout(binding=1) readonly buffer Indices { 
	uint data[]; 
} in_Indices;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;


// vertex shader
void main() {
	//const array of positions for the triangle
	uint idx = in_Indices.data[gl_VertexIndex];

	VertexData vtx = in_Vertices.data[idx];

	vec3 pos = vec3(vtx.x, vtx.y, vtx.z);


	//output the position of each vertex
	gl_Position =  PushConstants.render_matrix * vec4(pos, 1.0f);
}

#endif
#ifdef FRAGMENT_SHADER

layout (location = 0) out vec4 outFragColor;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;


// fragment shader
void main() {
  outFragColor = PushConstants.data;
}

#endif
