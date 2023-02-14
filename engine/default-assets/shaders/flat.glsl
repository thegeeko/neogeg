#version 450

#ifdef VERTEX_SHADER

// vertex shader
void main() {
	//const array of positions for the triangle
	const vec3 positions[3] = vec3[3](
		vec3(1.f,1.f, 0.0f),
		vec3(-1.f,1.f, 0.0f),
		vec3(0.f,-1.f, 0.0f)
	);

	//output the position of each vertex
	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
}

#endif
#ifdef FRAGMENT_SHADER

layout (location = 0) out vec4 outFragColor;

// fragment shader
void main() {
  outFragColor = vec4(1.f,0.f,0.f,1.0f);
}

#endif
