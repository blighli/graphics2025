#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;

out vec2 screenCoord;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	screenCoord = (vec2(aPos.x, aPos.y) + 1.0) / 2.0;

	gl_Position = vec4(aPos, 1.0f);
}


#shader fragment
#version 330 core
out vec4 FragColor;

#define TRIANGLE_SIEZ 12
#define BVHNODE_SIZE 4
#define INF 114514
#define PI 3.1415926535859

in vec2 screenCoord;

struct Light {
	vec3 Pos;
	vec3 color;
};

struct Material {
	vec3 emissive;
	vec3 baseColor;

	float subsurface;
	float roughness;
	float metallic;

	float specular;
	float specularTint;
	float anisotropic;

	float sheen;
	float sheenTine;
	float clearcoat;

	float clearcoatGloss;
};

struct Triangle {
	vec3 p1, p2, p3;
	vec3 n1, n2, n3;
};

struct Ray {
	vec3 start;
	vec3 dir;
};

struct HitResult {
	bool isHit;
	bool isInside;
	float distance;
	vec3 HitPoint;
	vec3 normal;
	vec3 viewDir;
	Material material;
};

struct Camera
{
	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;
	vec3 origin;
};

struct BVHNode {
	int left;
	int right;
	int n;
	int index;
	vec3 AA;
	vec3 BB;
};

uniform Camera camera;
uniform Light light;

uniform samplerBuffer triangles;
uniform samplerBuffer bvh;


// 读取BVH
BVHNode getBVHNode(int i)
{
	int offset = i * BVHNODE_SIZE;
	BVHNode node;

	ivec3 childs = ivec3(texelFetch(bvh, offset + 0).xyz);
	ivec3 leafInfo = ivec3(texelFetch(bvh, offset + 1).xyz);
	node.left = int(childs.x);
	node.right = int(childs.y);
	node.n = int(leafInfo.x);
	node.index = int(leafInfo.y);

	node.AA = texelFetch(bvh, offset + 2).xyz;
	node.BB = texelFetch(bvh, offset + 3).xyz;

	return node;
}

// 与AABB求交
float HitAABB(Ray r, vec3 AA, vec3 BB)
{
	vec3 invdir = 1.0 / r.dir;

	vec3 f = (BB - r.start) * invdir;
	vec3 n = (AA - r.start) * invdir;

	vec3 tmax = max(f, n);
	vec3 tmin = min(f, n);

	float t1 = min(tmax.x, min(tmax.y, tmax.z));
	float t0 = max(tmin.x, max(tmin.y, tmin.z));

	return (t1 >= t0) ? ((t0 > 0.0) ? (t0) : (t1)) : (-1);
}

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
	vec3 param1 = texelFetch(triangles, offset + 8).xyz;
	vec3 param2 = texelFetch(triangles, offset + 9).xyz;
	vec3 param3 = texelFetch(triangles, offset + 10).xyz;
	vec3 param4 = texelFetch(triangles, offset + 11).xyz;

	res.subsurface = param1.x;
	res.roughness = param1.y;
	res.metallic = param1.z;

	res.specular = param2.x;
	res.specularTint = param2.y;
	res.anisotropic = param2.z;

	res.sheen = param3.x;
	res.sheenTine = param3.y;
	res.clearcoat = param3.z;

	res.clearcoatGloss = param4.x;

	return res;
}

// 光线和三角形相交
HitResult HitTriangle(Triangle t, Ray ray)
{
	HitResult res;

	res.isHit = false;
	res.isInside = false;
	res.distance = INF;

	vec3 p1 = t.p1;
	vec3 p2 = t.p2;
	vec3 p3 = t.p3;

	vec3 S = ray.start;
	vec3 D = ray.dir;
	vec3 N = normalize(cross(p2 - p1, p3 - p1));
	//vec3 N = normalize(t.n1 + t.n2 + t.n3);

	// 光线从背面达到物体
	if (dot(N, D) > 0.0f) {
		N = -N;
		res.isInside = true;
	}

	// 平行三角形
	if (abs(dot(N, D)) < 0.00001f)
		return res;

	// 求交点
	float T = (dot(N, p1) - dot(S, N)) / dot(D, N);
	if (T < 0.001f) return res;

	// 交点
	vec3 P = S + D * T;

	// 判断是否在三角形内
	vec3 c1 = cross(p2 - p1, P - p1);
	vec3 c2 = cross(p3 - p2, P - p2);
	vec3 c3 = cross(p1 - p3, P - p3);
	bool res1 = (dot(c1, N) >= 0 && dot(c2, N) >= 0 && dot(c3, N) >= 0);
	bool res2 = (dot(c1, N) <= 0 && dot(c2, N) <= 0 && dot(c3, N) <= 0);

	//命中
	if (res1 || res2)
	{
		res.isHit = true;
		res.distance = T;
		res.HitPoint = P;
		res.viewDir = D;
		res.normal = N;

		//法线插值
		float alpha = (-(P.x - p2.x) * (p3.y - p2.y) + (P.y - p2.y) * (p3.x - p2.x)) / (-(p1.x - p2.x - 0.00005) * (p3.y - p2.y + 0.00005) + (p1.y - p2.y + 0.00005) * (p3.x - p2.x + 0.00005));
		float beta = (-(P.x - p3.x) * (p1.y - p3.y) + (P.y - p3.y) * (p1.x - p3.x)) / (-(p2.x - p3.x - 0.00005) * (p1.y - p3.y + 0.00005) + (p2.y - p3.y + 0.00005) * (p1.x - p3.x + 0.00005));
		float gama = 1.0 - alpha - beta;
		vec3 finalNormal = normalize(alpha * t.n1 + beta * t.n2 + gama * t.n3);
		res.normal = res.isInside ? -finalNormal : finalNormal;
	}

	return res;
}

// 遍历求最近的相交三角形
HitResult HitArray(Ray ray, int left, int right)
{
	HitResult res;
	res.isHit = false;
	res.distance = INF;

	for (int i = left; i <= right; i++)
	{
		Triangle t = getTriangle(i);

		HitResult tmp = HitTriangle(t, ray);
		if (tmp.isHit && tmp.distance < res.distance) {
			res = tmp;
			res.material = getMaterial(i);
		}
	}
	return res;
}

// 先与BVH求交
HitResult HitBVH(Ray ray)
{
	HitResult res;

	res.isHit = false;
	res.distance = INF;

	//实现栈
	int stack[256];
	int sp = 0;

	stack[sp++] = 1;
	while (sp > 0)
	{
		int top = stack[--sp];
		BVHNode node = getBVHNode(top);

		// 叶子节点
		if (node.n > 0)
		{
			int L = node.index;
			int R = node.index + node.n - 1;

			HitResult r = HitArray(ray, L, R);
			if (r.isHit && r.distance < res.distance)
				res = r;
			continue;
		}

		// 非叶子节点
		float dLeft = INF;// 左孩子距离
		float dRight = INF;	// 右孩子距离

		if (node.left > 0)
		{
			BVHNode leftNode = getBVHNode(node.left);
			dLeft = HitAABB(ray, leftNode.AA, leftNode.BB);
		}
		if (node.right > 0)
		{
			BVHNode rightNode = getBVHNode(node.right);
			dRight = HitAABB(ray, rightNode.AA, rightNode.BB);
		}

		if (dLeft > 0 && dRight > 0)
		{
			stack[sp++] = node.right;
			stack[sp++] = node.left;
		}
		else if (dLeft > 0)
		{
			stack[sp++] = node.left;
		}
		else if (dRight > 0)
		{
			stack[sp++] = node.right;
		}
	}
	return res;
}

vec3 phong(HitResult res)
{
	if (res.material.emissive != vec3(0))
		return res.material.emissive;

	// ambient
	float ka = 0.2;
	vec3 ambient = ka * light.color;

	// diffuse
	vec3 norm = normalize(res.normal);
	vec3 lightDir = normalize(light.Pos - res.HitPoint);

	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * light.color;

	// specular
	float ks = 0.5;
	vec3 viewDir = normalize(camera.origin - res.HitPoint);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = ks * spec * light.color;

	return (ambient + diffuse + specular) * res.material.baseColor;
}

Ray CameraGetRay(Camera camera, vec2 offset)
{
	Ray ray;
	ray.start = camera.origin;
	ray.dir = camera.lower_left_corner +
		offset.x * camera.horizontal +
		offset.y * camera.vertical - camera.origin;

	return ray;
}

void main()
{
	vec3 color = vec3(0);

	Ray ray = CameraGetRay(camera, screenCoord);

	HitResult r = HitBVH(ray);

	if (r.isHit)
		color = phong(r);

	FragColor = vec4(color, 1.0f);
}
