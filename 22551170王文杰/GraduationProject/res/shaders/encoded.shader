#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
}


#shader fragment
#version 330 core
out vec4 FragColor;

#define TRIANGLE_SIEZ 9

struct Material {
	vec3 emissive;
	vec3 baseColor;

	float ao;
	float roughness;
	float metallic;
};

struct Triangle {
	vec3 p1, p2, p3;
	vec3 n1, n2, n3;
};

uniform samplerBuffer triangles;
uniform int index;

Triangle getTriangle(int i) {
	int offset = i * TRIANGLE_SIEZ;
	Triangle res;

	// vertex position
	res.p1 = texelFetch(triangles, offset + 0).xyz;
	res.p2 = texelFetch(triangles, offset + 1).xyz;
	res.p3 = texelFetch(triangles, offset + 2).xyz;

	// normal position
	res.n1 = texelFetch(triangles, offset + 3).xyz;
	res.n2 = texelFetch(triangles, offset + 4).xyz;
	res.n3 = texelFetch(triangles, offset + 5).xyz;

	return res;
}

Material getMaterial(int i) {
	int offset = i * TRIANGLE_SIEZ;
	Material res;

	res.emissive = texelFetch(triangles, offset + 6).xyz;
	res.baseColor = texelFetch(triangles, offset + 7).xyz;
	vec3 tmp = texelFetch(triangles, offset + 8).xyz;
	
	res.ao = tmp.x;
	res.roughness = tmp.y;
	res.metallic = tmp.z;

	return res;
}

void main()
{
	Triangle t = getTriangle(index);
	Material m = getMaterial(index);

	FragColor = vec4(m.baseColor, 1.0f);
}
