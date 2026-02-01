#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;

out vec2 screenCoord;
out vec3 pix;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	//gl_Position = projection * view * vec4(aPos, 1.0f);
	gl_Position = vec4(aPos, 1.0f);

	screenCoord = (vec2(aPos.x, aPos.y) + 1.0) / 2.0;
	pix = aPos;
}


#shader fragment
#version 330 core
out vec4 FragColor;

in vec2 screenCoord;
in vec3 pix;

#define TRIANGLE_SIEZ 12
#define BVHNODE_SIZE 4
#define INF 114514
#define PI 3.1415926535859

// =========== Struct=============
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
	float refractive;
	float refraRatio;
};

struct Triangle {
	vec3 p1, p2, p3;
	vec3 n1, n2, n3;
};

struct Ray {
	vec3 start;
	vec3 dir;
	float refractive;
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

// ============= uniform===================
uniform Camera camera;
uniform samplerBuffer triangles;
uniform samplerBuffer bvh;
uniform int frameCount;
uniform sampler2D lastFrame;

uniform int width;
uniform int height;


// ============= funcion ==================
//=========================== Random =======================
uint seed = uint(
	uint(screenCoord.x * width) * uint(1973) +
	uint(screenCoord.y * height) * uint(9277) +
	uint(frameCount) * uint(26699)) | uint(1);

uint wang_hash(inout uint seed) {
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);
	return seed;
}


float rand() {
	return float(wang_hash(seed)) / 4294967296.0;
}

vec3 randVec3()
{
	return vec3(rand(), rand(), rand());
}

vec2 rand2()
{
	return vec2(rand(), rand());
}

const uint V[8 * 32] = uint[](
	2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 16777216, 8388608, 4194304, 2097152, 1048576, 524288, 262144, 131072, 65536, 32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1,
	2147483648, 3221225472, 2684354560, 4026531840, 2281701376, 3422552064, 2852126720, 4278190080, 2155872256, 3233808384, 2694840320, 4042260480, 2290614272, 3435921408, 2863267840, 4294901760, 2147516416, 3221274624, 2684395520, 4026593280, 2281736192, 3422604288, 2852170240, 4278255360, 2155905152, 3233857728, 2694881440, 4042322160, 2290649224, 3435973836, 2863311530, 4294967295,
	2147483648, 3221225472, 1610612736, 2415919104, 3892314112, 1543503872, 2382364672, 3305111552, 1753219072, 2629828608, 3999268864, 1435500544, 2154299392, 3231449088, 1626210304, 2421489664, 3900735488, 1556135936, 2388680704, 3314585600, 1751705600, 2627492864, 4008611328, 1431684352, 2147543168, 3221249216, 1610649184, 2415969680, 3892340840, 1543543964, 2382425838, 3305133397,
	2147483648, 3221225472, 536870912, 1342177280, 4160749568, 1946157056, 2717908992, 2466250752, 3632267264, 624951296, 1507852288, 3872391168, 2013790208, 3020685312, 2181169152, 3271884800, 546275328, 1363623936, 4226424832, 1977167872, 2693105664, 2437829632, 3689389568, 635137280, 1484783744, 3846176960, 2044723232, 3067084880, 2148008184, 3222012020, 537002146, 1342505107,
	2147483648, 1073741824, 536870912, 2952790016, 4160749568, 3690987520, 2046820352, 2634022912, 1518338048, 801112064, 2707423232, 4038066176, 3666345984, 1875116032, 2170683392, 1085997056, 579305472, 3016343552, 4217741312, 3719483392, 2013407232, 2617981952, 1510979072, 755882752, 2726789248, 4090085440, 3680870432, 1840435376, 2147625208, 1074478300, 537900666, 2953698205,
	2147483648, 1073741824, 1610612736, 805306368, 2818572288, 335544320, 2113929216, 3472883712, 2290089984, 3829399552, 3059744768, 1127219200, 3089629184, 4199809024, 3567124480, 1891565568, 394297344, 3988799488, 920674304, 4193267712, 2950604800, 3977188352, 3250028032, 129093376, 2231568512, 2963678272, 4281226848, 432124720, 803643432, 1633613396, 2672665246, 3170194367,
	2147483648, 3221225472, 2684354560, 3489660928, 1476395008, 2483027968, 1040187392, 3808428032, 3196059648, 599785472, 505413632, 4077912064, 1182269440, 1736704000, 2017853440, 2221342720, 3329785856, 2810494976, 3628507136, 1416089600, 2658719744, 864310272, 3863387648, 3076993792, 553150080, 272922560, 4167467040, 1148698640, 1719673080, 2009075780, 2149644390, 3222291575,
	2147483648, 1073741824, 2684354560, 1342177280, 2281701376, 1946157056, 436207616, 2566914048, 2625634304, 3208642560, 2720006144, 2098200576, 111673344, 2354315264, 3464626176, 4027383808, 2886631424, 3770826752, 1691164672, 3357462528, 1993345024, 3752330240, 873073152, 2870150400, 1700563072, 87021376, 1097028000, 1222351248, 1560027592, 2977959924, 23268898, 437609937
);

uint grayCode(uint i) {
	return i ^ (i >> 1);
}

float sobol(uint d, uint i)
{
	uint res = 0u;
	uint offset = d * 32u;
	for (uint j = 0u; i > 0u; i >>= 1u, j++)
		if ((i & 1u) > 0u)
			res ^= V[j + offset];

	return  float(res) * (1.0f / float(0xFFFFFFFFu));
}

vec2 sobolVec2(uint i, uint b) 
{
	float u = sobol(b * 2u, grayCode(i));
	float v = sobol(b * 2u + 1u, grayCode(i));
	return vec2(u, v);
}

vec2 CranleyPattersonRotation(vec2 p) 
{
	float u = float(wang_hash(seed)) / 4294967296.0;
	float v = float(wang_hash(seed)) / 4294967296.0;

	p.x += u;
	if (p.x > 1) p.x -= 1;
	if (p.x < 0) p.x += 1;

	p.y += v;
	if (p.y > 1) p.y -= 1;
	if (p.y < 0) p.y += 1;

	return p;
}

// --------- Sample----------------
vec3 myMul(vec3 v, mat3 m)
{
	return v.x * m[0] + v.y * m[1] + v.z* m[2];
}

float sdot(vec3 x, vec3 y, float f)
{
	return clamp(dot(x, y) * f, 0.0, 1.0);
}

mat3 GetTangentSpace(vec3 N)
{
	vec3 helper = vec3(1, 0, 0);
	if (abs(N.x) > 0.999)
		helper = vec3(0, 0, 1);
	
	vec3 tangent = normalize(cross(N, helper));
	vec3 binormal = normalize(cross(N, tangent));
	return mat3(tangent, binormal, N);
}

vec3 SampleHemisphere(vec3 normal) {
	float z = rand();
	float r = max(0, sqrt(1 - z * z));
	float phi = 2.0 * PI * rand();

	vec3 tangentSpaceDir = vec3(r * cos(phi), r * sin(phi), z);

	return myMul(tangentSpaceDir, GetTangentSpace(normal));
}

vec3 toNormalHemisphere(vec3 V, vec3 N)
{
	vec3 helper = vec3(1, 0, 0);
	if (abs(N.x) > 0.999) 
		helper = vec3(0, 0, 1);
	vec3 tangent = normalize(cross(N, helper));
	vec3 bitangent = normalize(cross(N, tangent));
	return V.x * tangent + V.y * bitangent + V.z * N;
}

vec3 SampleCosHemisphere(vec3 normal, float x1, float x2)
{
	float r = sqrt(x1);
	float theta = x2 * 2.0 * PI;

	float x = r * cos(theta);
	float y = r * sin(theta);

	float z = sqrt(1.0 - x * x - y * y);

	vec3 L = toNormalHemisphere(vec3(x, y, z), normal);

	return L;
}

vec3 SampleGTR1(vec3 V, vec3 N, float x1, float x2, float alpha)
{
	float phi_h = 2.0 * PI * x1;
	float sin_phi_h = sin(phi_h);
	float cos_phi_h = cos(phi_h);

	float cos_theta_h = sqrt((1.0 - pow(alpha * alpha, 1.0 - x2)) / (1.0 - alpha * alpha));
	float sin_theta_h = sqrt(max(0.0, 1.0 - cos_theta_h * cos_theta_h));

	vec3 H = vec3(sin_theta_h * cos_phi_h, sin_theta_h * sin_phi_h, cos_theta_h);
	H = toNormalHemisphere(H, N);   

	return H;
}

vec3 SampleGTR2(vec3 V, vec3 N, float x1, float x2, float alpha)
{
	float phi_h = 2.0 * PI * x1;
	float sin_phi_h = sin(phi_h);
	float cos_phi_h = cos(phi_h);

	float cos_theta_h = sqrt((1.0 - x2) / (1.0 + (alpha * alpha - 1.0) * x2));
	float sin_theta_h = sqrt(max(0.0, 1.0 - cos_theta_h * cos_theta_h));

	vec3 H = vec3(sin_theta_h * cos_phi_h, sin_theta_h * sin_phi_h, cos_theta_h);
	H = toNormalHemisphere(H, N);   

	return H;
}

vec3 SampleBRDF(float x1, float x2, float p, vec3 V, vec3 N, in Material material)
{
	float alpha_GTR1 = mix(0.1, 0.001, material.clearcoatGloss);
	float alpha_GTR2 = max(0.001, material.roughness * material.roughness);

	float r_diffuse = 1.0 - material.metallic;
	float r_specular = 1.0;
	float r_clearcoat = 0.25 * material.clearcoat;

	float r_sum = r_diffuse + r_specular + r_clearcoat;

	float diffuseThreshold = r_diffuse / r_sum;
	float specularThreshold = r_specular / r_sum;
	float clearcoatThreshold = r_clearcoat / r_sum;
	
	vec3 result = vec3(0, 1, 0);

	if (p <= diffuseThreshold) 
	{
		result = SampleCosHemisphere(N, x1, x2);
	}
	else if (diffuseThreshold < p && p <= diffuseThreshold + specularThreshold) 
	{
		vec3 H = SampleGTR2(V, N, x1, x2, alpha_GTR2);
		result = reflect(-V, H);
	}
	else if (p > diffuseThreshold + specularThreshold) 
	{
		vec3 H = SampleGTR1(V, N, x1, x2, alpha_GTR1);
		result = reflect(-V, H);
	}
	
	return result;
}

vec3 SampleBRDF_Refrac(float x1, float x2, float p, vec3 V, vec3 N, in Material material, float eta)
{
	float alpha_GTR1 = mix(0.1, 0.001, material.clearcoatGloss);
	float alpha_GTR2 = max(0.001, material.roughness * material.roughness);

	float r_diffuse = 1.0 - material.metallic;
	float r_specular = 1.0;
	float r_clearcoat = 0.25 * material.clearcoat;

	float r_sum = r_diffuse + r_specular + r_clearcoat;

	float diffuseThreshold = r_diffuse / r_sum;
	float specularThreshold = r_specular / r_sum;
	float clearcoatThreshold = r_clearcoat / r_sum;
	
	vec3 result = vec3(0, 1, 0);

	if (p <= diffuseThreshold) 
	{
		result = SampleCosHemisphere(-N, x1, x2);
	}
	else if (diffuseThreshold < p && p <= diffuseThreshold + specularThreshold) 
	{
		vec3 H = SampleGTR2(V, N, x1, x2, alpha_GTR2);
		result = refract(-V, H, eta);
	}
	else if (p > diffuseThreshold + specularThreshold) 
	{
		vec3 H = SampleGTR1(V, N, x1, x2, alpha_GTR1);
		result = refract(-V, H, eta);
	}
	
	return result;
}

vec3 SampleBRDF_Clearcoat(float x1, float x2, float p, vec3 V, vec3 N, in Material material)
{
	float alpha_GTR1 = mix(0.1, 0.001, material.clearcoatGloss);

	float r_diffuse = 1.0 - material.metallic;
	float r_clearcoat = 0.25 * material.clearcoat;

	float r_sum = r_diffuse + r_clearcoat;

	float diffuseThreshold = r_diffuse / r_sum;
	float clearcoatThreshold = r_clearcoat / r_sum;

	if (p <= diffuseThreshold)
	{
		return SampleCosHemisphere(N, x1, x2);
	}
	else if (p > diffuseThreshold)
	{
		return SampleGTR1(V, N, x1, x2, alpha_GTR1);
	}

	return vec3(0, 1, 0);
}

vec3 SampleBRDF_Specular(float x1, float x2, float p, vec3 V, vec3 N, in Material material)
{
	float alpha_GTR2 = max(0.001, material.roughness * material.roughness);

	float r_diffuse = 1.0 - material.metallic;
	float r_specular = 1.0;	

	float r_sum = r_diffuse + r_specular;

	float diffuseThreshold = r_diffuse / r_sum;
	float specularThreshold = r_specular / r_sum;

	if (p <= diffuseThreshold)
	{
		return SampleCosHemisphere(N, x1, x2);
	}
	else if (diffuseThreshold < p) 
	{
		return SampleGTR2(V, N, x1, x2, alpha_GTR2);
	}

	return vec3(0, 1, 0);
}

//==========================================

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
	res.refractive = param4.y;
	res.refraRatio = param4.z;

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
	if(res.material.refractive == ray.refractive)
	{
		res.material.refractive = 1.0;
	}
	return res;
}


//============PRB===================
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float schlickFresnel(float cosTheta)
{
	return pow(clamp(1 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 PBR(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0) 
		return vec3(0);


	vec3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, material.baseColor, material.metallic);

	// 菲涅尔项
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float NDF = DistributionGGX(N, H, material.roughness);
	float G = GeometrySmith(N, V, L, material.roughness);

	vec3 nominator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
	vec3 specular = nominator / denominator;

	vec3 ks = F;
	vec3 kd = vec3(1.0) - ks;

	kd *= 1.0 - material.metallic;

	return kd * material.baseColor / PI + specular;
}

float SchlickFresnel(float u)
{
	float m = clamp(1 - u, 0, 1);
	float m2 = m * m;
	return m2 * m2 * m;
}

float GTR1(float NdotH, float a) {
	if (a >= 1) return 1 / PI;
	float a2 = a * a;
	float t = 1.0 + (a2 - 1) * NdotH * NdotH;
	return (a2 - 1) / (PI * log(a2) * t );
}

float GTR2(float NdotH, float a) {
	float a2 = a * a;
	float t = 1 + (a2 - 1) * NdotH * NdotH;
	return a2 / (PI * t * t);
}

float smithG_GGX(float NdotV, float alphaG) {
	float a = alphaG * alphaG;
	float b = NdotV * NdotV;
	return 1 / (NdotV + sqrt(a + b - a * b));
}

vec3 mon2lin(vec3 x)
{
	return vec3(pow(x[0], 2.2), pow(x[1], 2.2), pow(x[2], 2.2));
}


float getPDF(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0)
		return 0;

	vec3 H = normalize(L + V);
	float NdotH = max(0.01, dot(N, H));
	float LdotH = max(0.01, dot(L, H));

	float alpha_GTR1 = mix(0.1, 0.001, material.clearcoatGloss);
	float alpha_GTR2 = max(0.001, material.roughness * material.roughness);

	// 计算概率
	float r_diffuse = 1.0 - material.metallic;
	float r_specular = 1.0;
	float r_clearcoat = 0.25 * material.clearcoat;

	float r_sum = r_diffuse + r_specular +r_clearcoat;

	float diffuseThreshold = r_diffuse / r_sum;
	float specularThreshold = r_specular / r_sum;
	float clearcoatThreshold = r_clearcoat / r_sum;

	// 计算pdf
	float Ds = GTR2(NdotH, alpha_GTR2);
	float Dr = GTR1(NdotH, alpha_GTR1);

	float pdf_diffuse = NdotL / PI;
	float pdf_specular = Ds * NdotH / (4.0 * LdotH);
	float pdf_clearcoat = Dr * NdotH / (4.0 * LdotH);


	// 混合
	float pdf = diffuseThreshold * pdf_diffuse
		+ specularThreshold * pdf_specular
		+ clearcoatThreshold * pdf_clearcoat;
	pdf = max(1e-10, pdf);

	return pdf;
}

float getPDF_Clearcoat(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0)
		return 0;

	vec3 H = normalize(L + V);
	float NdotH = max(0.01, dot(N, H));
	float LdotH = max(0.01, dot(L, H));

	float alpha_GTR1 = mix(0.1, 0.001, material.clearcoatGloss);

	// 计算概率
	float r_diffuse = 1.0 - material.metallic;
	float r_clearcoat = 0.25 * material.clearcoat;

	float r_sum = r_diffuse + r_clearcoat;

	float diffuseThreshold = r_diffuse / r_sum;
	float clearcoatThreshold = r_clearcoat / r_sum;

	// 计算pdf
	float Dr = GTR1(NdotH, alpha_GTR1);

	float pdf_diffuse = NdotL / PI;
	float pdf_clearcoat = Dr * NdotH / (4.0 * LdotH);

	// 混合
	float pdf = diffuseThreshold * pdf_diffuse
		+ clearcoatThreshold * pdf_clearcoat;
	pdf = max(1e-10, pdf);

	return pdf;
}

float getPDF_Specular(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0)
		return 0;

	vec3 H = normalize(L + V);
	float NdotH = max(0.01, dot(N, H));
	float LdotH = max(0.01, dot(L, H));

	float alpha_GTR2 = max(0.001, material.roughness * material.roughness);

	// 计算概率
	float r_diffuse = 1.0 - material.metallic;
	float r_specular = 1.0;

	float r_sum = r_diffuse + r_specular;

	float diffuseThreshold = r_diffuse / r_sum;
	float specularThreshold = r_specular / r_sum;

	// 计算pdf
	float Ds = GTR2(NdotH, alpha_GTR2);

	float pdf_diffuse = NdotL / (PI);
	float pdf_specular = Ds * NdotH / (4.0 * LdotH);


	// 混合
	float pdf = diffuseThreshold * pdf_diffuse
		+ specularThreshold * pdf_specular;

	pdf = max(1e-10, pdf);

	return pdf;
}

vec3 Disney_BRDF(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0)
		return vec3(0);

	vec3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);
	
	vec3 Cdlin = mon2lin(material.baseColor);

	// diffuse
	float Fd90 = 0.5 + 2.0 * material.roughness * LdotH * LdotH;
	float FL = schlickFresnel(NdotL);
	float FV = schlickFresnel(NdotV);

	float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

	float Fss90 = LdotH * LdotH * material.roughness;
	float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
	float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);

	vec3 diffuse = Cdlin / PI * mix(Fd, ss, material.subsurface) * (1 - material.metallic);

	// specular
	float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z;
	vec3 Ctint = (Cdlum) > 0 ? (Cdlin / Cdlum) : vec3(1);
	vec3 Cspec = material.specular * mix(vec3(1), Ctint, material.specularTint);
	vec3 Cspec0 = mix(0.08 * Cspec, Cdlin, material.metallic);
	
	float alpha = max(0.001, material.roughness * material.roughness);
	float Ds = GTR2(NdotH, alpha);
	float FH = SchlickFresnel(LdotH);
	vec3 Fs = mix(Cspec0, vec3(1), FH);
	float Gs = smithG_GGX(NdotL, material.roughness);
	Gs *= smithG_GGX(NdotV, material.roughness);

	vec3 specular = Gs * Fs * Ds;

	// 清漆
	float Dr = GTR1(NdotH, mix(0.1, 0.001, material.clearcoatGloss));
	float Fr = mix(0.04, 1.0, FH);
	float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);
	
	vec3 clearcoat = vec3(0.25 * Gr * Fr * Dr * material.clearcoat);

	//sheen
	vec3 Csheen = mix(vec3(1), Ctint, material.sheenTine);
	vec3 Fsheen = FH * material.sheen * Csheen;

	diffuse += Fsheen;

	return diffuse + specular + clearcoat;
}

vec3 Disney_BRDF_Clearcoat(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0)
		return vec3(0);

	vec3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);

	vec3 Cdlin = mon2lin(material.baseColor);

	// diffuse
	float Fd90 = 0.5 + 2.0 * material.roughness * LdotH * LdotH;
	float FL = schlickFresnel(NdotL);
	float FV = schlickFresnel(NdotV);

	float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

	float Fss90 = LdotH * LdotH * material.roughness;
	float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
	float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);

	vec3 diffuse = Cdlin / PI * mix(Fd, ss, material.subsurface) * (1 - material.metallic);

	// 清漆
	float FH = SchlickFresnel(LdotH);
	float Dr = GTR1(NdotH, mix(0.1, 0.001, material.clearcoatGloss));
	float Fr = mix(0.04, 1.0, FH);
	float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);

	vec3 clearcoat = vec3(0.25 * Gr * Fr * Dr * material.clearcoat);

	return diffuse + clearcoat;
}

vec3 Disney_BRDF_Specular(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0)
		return vec3(0);

	vec3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);

	vec3 Cdlin = mon2lin(material.baseColor);

	// diffuse
	float Fd90 = 0.5 + 2.0 * material.roughness * LdotH * LdotH;
	float FL = schlickFresnel(NdotL);
	float FV = schlickFresnel(NdotV);

	float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

	float Fss90 = LdotH * LdotH * material.roughness;
	float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
	float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);

	vec3 diffuse = Cdlin / PI * mix(Fd, ss, material.subsurface) * (1 - material.metallic);

	// specular
	float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z;
	vec3 Ctint = (Cdlum) > 0 ? (Cdlin / Cdlum) : vec3(1);
	vec3 Cspec = material.specular * mix(vec3(1), Ctint, material.specularTint);
	vec3 Cspec0 = mix(0.08 * Cspec, Cdlin, material.metallic);

	float alpha = max(0.001, material.roughness * material.roughness);
	float Ds = GTR2(NdotH, alpha);
	float FH = SchlickFresnel(LdotH);
	vec3 Fs = mix(Cspec0, vec3(1), FH);
	float Gs = smithG_GGX(NdotL, material.roughness);
	Gs *= smithG_GGX(NdotV, material.roughness);

	vec3 specular = Gs * Fs * Ds;

	return diffuse + specular;
}

vec3 Disney_BRDF_Diffuse(vec3 V, vec3 N, vec3 L, in Material material)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	if (NdotL < 0 || NdotV < 0)
		return vec3(0);

	vec3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);

	vec3 Cdlin = mon2lin(material.baseColor);

	// diffuse
	float Fd90 = 0.5 + 2.0 * material.roughness * LdotH * LdotH;
	float FL = schlickFresnel(NdotL);
	float FV = schlickFresnel(NdotV);

	float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

	float Fss90 = LdotH * LdotH * material.roughness;
	float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
	float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);

	vec3 diffuse = Cdlin / PI * mix(Fd, ss, material.subsurface) * (1 - material.metallic);

	return diffuse;
}
//============================
vec3 pathTracing(HitResult hit, float RR) {
	vec3 Lo = vec3(0);
	vec3 history = vec3(1);

	float P = rand();
	float lasRefractive = 1.0;
	
	// RR 算法
	while (P < RR)
	{	
		P = rand();

		Ray ray;
		ray.start = hit.HitPoint;
		ray.refractive = hit.material.refractive;

		vec3 V = normalize(-hit.viewDir);
		vec3 N = normalize(hit.normal);

		//vec2 rd2 = CranleyPattersonRotation(sobolVec2(uint(frameCount+1), uint(reflectCount)));
		float x1 = rand();
		float x2 = rand();
		float x3 = rand();

		vec3 L = vec3(0);
		vec3 f_r = vec3(0);
		float NdotL = 0;
		float pdf = 1;
		
		L = SampleBRDF(x1, x2, x3, V, N, hit.material);
		f_r = Disney_BRDF(V, N, L, hit.material);
		pdf = getPDF(V, N, L, hit.material);
		
		NdotL = dot(N, L);
		float NdotV = dot(N, V);

		if (NdotL < 0.0)
			break;


		if(abs(rand()) < hit.material.refraRatio)
		{
			float eta = lasRefractive / hit.material.refractive;
			L = L - (1. + eta) * NdotL * N;
		}

		ray.dir = L;

		if (pdf <= 0.0) break;
		
		HitResult newHit = HitBVH(ray);
		if (!newHit.isHit)
			break;


		vec3 Le = newHit.material.emissive;

		history *= f_r * NdotL / pdf;
	
		Lo += history * Le;
		lasRefractive = ray.refractive; 
		hit = newHit;	
	}
	return Lo / RR;
}

vec3 pathTracing_BSDF(HitResult hit, float RR) {
	vec3 Lo = vec3(0);
	vec3 history = vec3(1);

	float P = rand();
	float lasRefractive = 1.0;
	
	// RR 算法
	while (P < RR)
	{	
		P = rand();

		Ray ray;
		ray.start = hit.HitPoint;
		ray.refractive = hit.material.refractive;

		vec3 V = normalize(-hit.viewDir);
		vec3 N = normalize(hit.normal);

		//vec2 rd2 = CranleyPattersonRotation(sobolVec2(uint(frameCount+1), uint(reflectCount)));
		float x1 = rand();
		float x2 = rand();
		float x3 = rand();

		vec3 L = vec3(0);
		vec3 f_r = vec3(0);
		float NdotL = 0;
		float pdf = 1;
		
		L = SampleCosHemisphere(N, x1, x2);
		f_r = PBR(V, N, L, hit.material);
		pdf = 1. / PI;
		
		NdotL = dot(N, L);
		float NdotV = dot(N, V);

		if (NdotL < 0.0)
			break;


		if(abs(rand()) < hit.material.refraRatio)
		{
			L = L + 2 * NdotL * N;
		}

		ray.dir = L;

		if (pdf <= 0.0) break;
		
		HitResult newHit = HitBVH(ray);
		if (!newHit.isHit)
			break;


		vec3 Le = newHit.material.emissive;

		history *= f_r * NdotL / pdf;
	
		Lo += history * Le;
		lasRefractive = ray.refractive; 
		hit = newHit;	
	}
	return Lo / RR;
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
	float u = screenCoord.x;
	float v = screenCoord.y;
	
	Ray ray = CameraGetRay(camera, screenCoord);
	
	HitResult r = HitBVH(ray);

	vec3 color = vec3(0);	
	
	if (r.isHit)
	{	
		color = pathTracing(r, 0.8);
	}
	color += r.material.emissive;
	// 与上一拟合
	vec3 lastColor = texture(lastFrame, screenCoord.xy).rgb;
	color = mix(lastColor, color, 1.0 / float(frameCount + 1));

	FragColor = vec4(color, 1);
}