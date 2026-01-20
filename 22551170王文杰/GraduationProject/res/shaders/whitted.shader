#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 pix;
out vec2 screenCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	pix = aPos;
	screenCoord = (vec2(aPos.x, aPos.y) + 1.0) / 2.0;
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
}


#shader fragment
#version 330 core
out vec4 FragColor;

in vec3 pix;

#define TRIANGLE_SIEZ 9
#define INF 12138
#define PI 3.1415926535859

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

//============ Plan ==============
struct Model {
	int begin, end; // 模型三角形的起始和最后

	// TODO: AABB
};

in vec2 screenCoord;

uniform uint frameCount;
uniform vec2 screenSize;

uniform mat4 cameraRotate;

uint wseed = uint(
	uint((pix.x * 0.5 + 0.5) * screenSize.x) * uint(1973) +
	uint((pix.y * 0.5 + 0.5) * screenSize.y) * uint(9277) +
	uint(frameCount) * uint(26699)) | uint(1);

//================================

uniform samplerBuffer triangles;
uniform int nums;
uniform vec3 cameraPos;

//=========================== Random =======================
uint whash(uint seed)
{
	seed = (seed ^ uint(61)) ^ (seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> uint(4));
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> uint(15));
	return seed;
}

float randcore4()
{
	wseed = whash(wseed);

	return float(wseed) * (1.0 / 4294967296.0);
}


float rand()
{
	return randcore4();
}

vec3 randVec3()
{
	return vec3(rand(), rand(), rand());
}

vec2 rand2()
{
	return vec2(rand(), rand());
}

vec3 SampleHemisphere() {
	float z = rand();
	float r = max(0, sqrt(1 - z * z));
	float phi = 2.0 * PI * rand();
	return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 toNormalHemisphere(vec3 V, vec3 N)
{
	vec3 helper = vec3(1, 0, 0);
	if (abs(N.x) > 0.99) helper = vec3(0, 0, 1);
	vec3 tangent = normalize(cross(N, helper));
	vec3 bitangent = normalize(cross(N, tangent));
	return V.x * tangent + V.y * bitangent + V.z * N;
}

struct Camera
{
	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;
	vec3 origin;
};

uniform Camera camera;


//===========================================================


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

// 光线和三角形相交
HitResult TriangleIntersect(Triangle t, Ray ray)
{
	//bool isHit;
	//bool isInside;
	//float distance;
	//vec3 HitPoint;
	//vec3 normal;
	//vec3 viewDir;
	//Material material;

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

	// 光线从背面达到物体
	if (dot(N, D) > 0.0f) {
		res.isInside = true;
		N = -N;
	}

	// 平行三角形
	if (abs(dot(N, D)) < 0.00005f)
		return res;

	float T = (dot(N, p1) - dot(S, N)) / dot(D, N);
	if (T < 0.0005f) return res;

	// 交点
	vec3 P = S + T * D;

	// 判断是否在三角形内
	vec3 c1 = cross(p2 - p1, P - p1);
	vec3 c2 = cross(p3 - p2, P - p2);
	vec3 c3 = cross(p1 - p3, P - p3);
	bool res1 = (dot(c1, N) > 0 && dot(c2, N) > 0 && dot(c3, N) > 0);
	bool res2 = (dot(c1, N) < 0 && dot(c2, N) < 0 && dot(c3, N) < 0);

	//命中
	if (res1 || res2)
	{
		res.isHit = true;
		res.distance = T;
		res.HitPoint = P;
		res.viewDir = D;
		
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
		
		HitResult tmp = TriangleIntersect(t, ray);
		if (tmp.isHit && tmp.distance < res.distance) {
			res = tmp;
			res.material = getMaterial(i);
		}
	}
	return res;
}

//============ pathTracing==============
vec3 pathTracing(HitResult hit, int maxBounce) {
	vec3 Lo = vec3(0);
	vec3 history = vec3(1);

	float RR = rand();
	while (maxBounce > 0)
	{
		vec3 wi = toNormalHemisphere(SampleHemisphere(), hit.normal);

		Ray ray;
		ray.start = hit.HitPoint;
		ray.dir = wi;

		HitResult newHit = HitArray(ray, 0, nums);

		if (!newHit.isHit)
			break;

		float pdf = 1.0 / (2.0 * PI);                                   // 半球均匀采样概率密度
		float cosine_o = max(0, dot(-hit.viewDir, hit.normal));         // 入射光和法线夹角余弦
		float cosine_i = max(0, dot(-ray.dir, hit.normal));				// 出射光和法线夹角余弦
		vec3 f_r = hit.material.baseColor / PI;

		vec3 Le = newHit.material.emissive;
		Lo += history * Le * f_r * cosine_i / (pdf * 1);

		hit = newHit;
		history *= f_r * cosine_i / (pdf * 1);
		RR = rand();
		maxBounce--;
	}
	return Lo;
}
//======================================

vec3 WorldTrace(Ray ray)
{
	HitResult res = HitArray(ray, 0, nums);

	if (res.isHit && !res.isInside)
	{
		vec3 N = res.normal;
		return 0.5 * vec3(N.x + 1, N.y + 1, N.z + 1);
	}
	else
	{
		vec3 unit_direction = normalize(ray.dir);
		float t = 0.5 * (unit_direction.y + 1.0);
		return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
		//return vec3(0);
	}

}

Ray CameraGetRay(Camera camera, vec2 offset)
{
	Ray ray;
	ray.start = camera.origin;
	ray.dir = normalize(camera.lower_left_corner + offset.x * camera.horizontal + offset.y * camera.vertical);

	return ray;
}

void main()
{
	float u = screenCoord.x;
	float v = screenCoord.y;

	vec3 col = vec3(0.0, 0.0, 0.0);
	int ns = 5;
	for (int i = 0; i < ns; i++)
	{
		Ray ray = CameraGetRay(camera, vec2(u, v) + rand2() / screenSize);
		col += WorldTrace(ray);
	}
	col /= ns;

	Ray ray2;
	ray2.start = camera.origin;
	ray2.dir = camera.lower_left_corner +
		u * camera.horizontal +
		v * camera.vertical - camera.origin;

	HitResult hit = HitArray(ray2, 0, nums);
	vec3 color = vec3(0);
	if (hit.isHit) {
		vec3 Le = hit.material.emissive;
		vec3 Li = pathTracing(hit, 2);
		color = Le + Li;
	}
	//FragColor.xyz = WorldTrace(ray2);
	//FragColor.xyz = color;
	FragColor.xyz = col;
	FragColor.w = 1.0;
}
