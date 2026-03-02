#pragma once
#include <ostream>
#include <cassert>

#pragma region 向量相关
/*———————————————————————————————————向量相关——————————————————————————————————*/
// 向量模板类，及相关方法
template<typename T, size_t size>
class Vec {
	T data[size];
public:
	Vec() {
		for (size_t i = 0; i < size; i++) {
			data[i] = T(0);
		}
	}

	// 随机访问
	T operator[](const size_t i) const {
		assert(i < size);
		return data[i];
	}
	T& operator[](const size_t i) {
		assert(i < size);
		return data[i];
	}
	// 平方和/长度/归一化
	T norm2() const {
		T sum = 0;
		for (size_t i = 0; i < size; i++) {
			sum += data[i] * data[i];
		}
		return sum;
	}
	T norm() const {
		return std::sqrt(norm2());
	}
	Vec<T, size> normalized() {
		return (*this) / norm();
	}
};

// 二维向量具体化
template<typename T>
class Vec<T, 2> {
public:
	static Vec<T, 2> Zero, One;
	union {
		T x;
		T u;
	};
	union {
		T y;
		T v;
	};
	Vec() : x(0), y(0) {};
	Vec(T _x, T _y) : x(_x), y(_y) {};

	Vec<T, 2>& operator=(const Vec<T, 2>& rhs) {
		x = rhs.x;
		y = rhs.y;
		return (*this);
	}
	T operator[](const size_t i) const {
		assert(i >= 0 && i < 2);
		return i ? y : x;
	}
	T& operator[](const size_t i) {
		assert(i >= 0 && i < 2);
		return i ? y : x;
	}
	T norm2() {
		return x * x + y * y;
	}
	T norm() {
		return std::sqrt(norm2());
	}
	Vec<T, 2> normalize() {
		return (*this) / norm();
	}
};
template<> Vec<float, 2> Vec<float, 2>::Zero = Vec<float, 2>{ 0, 0 };
template<> Vec<float, 2> Vec<float, 2>::One = Vec<float, 2>{ 1, 1 };

// 三维向量具体化
template<typename T>
class Vec<T, 3> {
public:
	static Vec<T, 3> Zero, One;
	T x, y, z;
	Vec() : x(0), y(0), z(0) {};
	Vec(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {};
	Vec(Vec<T, 2> _xy, T _z) : x(_xy.x), y(_xy.y), z(_z) {};

	Vec<T, 3>& operator=(const Vec<T, 3>& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return (*this);
	}
	T operator[](const size_t i) const {
		assert(i < 3);
		return i == 0 ? x : i == 1 ? y : z;
	}
	T& operator[](const size_t i) {
		assert(i < 3);
		return i == 0 ? x : i == 1 ? y : z;
	}
	T norm2() {
		return x * x + y * y + z * z;
	}
	T norm() {
		return std::sqrt(norm2());
	}
	Vec<T, 3> normalize() {
		return (*this) / norm();
	}
};
template<> Vec<float, 3> Vec<float, 3>::Zero = Vec<float, 3>{ 0, 0, 0 };
template<> Vec<float, 3> Vec<float, 3>::One = Vec<float, 3>{ 1, 1, 1 };
// 四维向量具体化
template<typename T>
class Vec<T, 4> {
public:
	static Vec<T, 4> Zero, One;
	union {
		T x;
		T r;
	};
	union {
		T y;
		T g;
	};
	union {
		T z;
		T b;
	};
	union {
		T w;
		T a;
	};
	Vec() : x(0), y(0), z(0), w(0) {};
	Vec(T _x, T _y, T _z, T _w = T()) : x(_x), y(_y), z(_z), w(_w) {};
	Vec(Vec<T, 2> _xy, T _z, T _w) : x(_xy.x), y(_xy.y), z(_z), w(_w) {};
	Vec(Vec<T, 3> _xyz, T _w) : x(_xyz.x), y(_xyz.y), z(_xyz.z), w(_w) {};

	Vec<T, 4>& operator=(const Vec<T, 4>& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
		return (*this);
	}
	T operator[](const size_t i) const {
		assert(i < 4);
		return i == 0 ? x : i == 1 ? y : i == 2 ? z : w;
	}
	T& operator[](const size_t i) {
		assert(i < 4);
		return i == 0 ? x : i == 1 ? y : i == 2 ? z : w;
	}
	T norm2() {
		return x * x + y * y + z * z + w * w;
	}
	T norm() {
		return std::sqrt(norm2());
	}
	Vec<T, 4> normalize() {
		return (*this) / norm();
	}
};
template<> Vec<float, 4> Vec<float, 4>::Zero = Vec<float, 4>{ 0, 0, 0, 0 };
template<> Vec<float, 4> Vec<float, 4>::One = Vec<float, 4>{ 1, 1, 1, 1 };

// 高维转低维
template<size_t target_size, size_t src_size, typename T>
Vec<T, target_size> proj(const Vec<T, src_size>& src) {
	assert(target_size < src_size);
	Vec<T, target_size> ret;
	for (int i = 0; i < target_size; i++) {
		ret[i] = src[i];
	}
	return ret;
}

// 常数：加减乘除
template <typename T, size_t size>
Vec<T, size> operator+(const T lhs, const Vec<T, size>& rhs) {
	Vec<T, size> res;
	for (size_t i = 0; i < size; i++) {
		res[i] = rhs[i] + lhs;
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator+(const Vec<T, size>& lhs, const T rhs) {
	Vec<T, size> res;
	for (size_t i = 0; i < size; i++) {
		res[i] = lhs[i] + rhs;
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator-(const Vec<T, size>& lhs, const T rhs) {
	Vec<T, size> res;
	for (size_t i = 0; i < size; i++) {
		res[i] = lhs[i] - rhs;
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator-(const Vec<T, size>& lhs) {
	Vec<T, size> res;
	for (size_t i = 0; i < size; i++) {
		res[i] = -lhs[i];
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator*(const T lhs, const Vec<T, size>& rhs) {
	Vec<T, size> res;
	for (size_t i = 0; i < size; i++) {
		res[i] = lhs * rhs[i];
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator*(const Vec<T, size>& lhs, const T rhs) {
	Vec<T, size> res;
	for (size_t i = 0; i < size; i++) {
		res[i] = lhs[i] * rhs;
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator/(const Vec<T, size>& lhs, const T rhs) {
	Vec<T, size> res;
	for (size_t i = 0; i < size; i++) {
		res[i] = lhs[i] / rhs;
	}
	return res;
}
// 变量逐元素：加减乘（点乘）/叉乘，赋值
template <typename T, size_t size>
Vec<T, size> operator+(const Vec<T, size>& lhs, const Vec<T, size> rhs) {
	Vec<T, size> res = lhs;
	for (size_t i = 0; i < size; i++) {
		res[i] += rhs[i];
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator-(const Vec<T, size>& lhs, const Vec<T, size> rhs) {
	Vec<T, size> res = lhs;
	for (size_t i = 0; i < size; i++) {
		res[i] -= rhs[i];
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> operator*(const Vec<T, size>& lhs, const Vec<T, size> rhs) {
	Vec<T, size> res = lhs;
	for (size_t i = 0; i < size; i++) {
		res[i] *= rhs[i];
	}
	return res;
}
template <typename T, size_t size>
T dot(const Vec<T, size>& lhs, const Vec<T, size> rhs) {
	T res = 0;
	for (size_t i = 0; i < size; i++) {
		res += lhs[i] * rhs[i];
	}
	return res;
}
template <typename T, size_t size>
Vec<T, size> cross(const Vec<T, size>& lhs, const Vec<T, size>& rhs) {
	Vec<T, size> res{
		lhs[1] * rhs[2] - lhs[2] * rhs[1],
		lhs[2] * rhs[0] - lhs[0] * rhs[2],
		lhs[0] * rhs[1] - lhs[1] * rhs[0]
	};

	return res;
}

// 输出流输出重载
template <typename T, size_t size>
std::ostream& operator<<(std::ostream& out, const Vec<T, size> v) {
	for (size_t i = 0; i < size; i++) {
		out << v[i] << ' ';
	}
	return out;
}

template<typename T, size_t size>
Vec<T, size> clamp(T left, T right ,const Vec<T, size>& src) {
	if (left > right) {
		std::swap(left, right);
	}
	Vec<T, size> ret = src;
	for (size_t i = 0; i < size; i++) {
		ret[i] = ret[i] < left ? left : (ret[i] > right ? right : ret[i]);
	}

	return ret;
}


typedef Vec<int, 2> vec2i;
typedef Vec<float, 2> vec2f;
typedef Vec<int, 3> vec3i;
typedef Vec<float, 3> vec3f;
typedef Vec<int, 4> vec4i;
typedef Vec<float, 4> vec4f;
#pragma endregion

namespace Color {
	const Vec<float, 4> White{ 1.f, 1.f, 1.f, 1.f };
	const Vec<float, 4> Black{ 0.f, 0.f, 0.f, 1.f };
	const Vec<float, 4> Red{ 1.f, 0.f, 0.f, 1.f };
	const Vec<float, 4> Blue{ 0.f, 0.f, 1.f, 1.f };
	const Vec<float, 4> Green{ 0.f, 1.f, 0.f, 1.f };
}

#pragma region 矩阵相关
/*———————————————————————————————————矩阵相关——————————————————————————————————*/
// 矩阵模板类，及相关方法
template<typename T, size_t row_size, size_t col_size>
class Mat {
private:
	Vec<T, col_size> data[row_size];
public:
	Mat() {
		for (int i = 0; i < row_size; i++) {
			for (int j = 0; j < col_size; j++) {
				data[i][j] = 0;
			}
		}
	}
	Mat(const Vec<T, col_size>(&src)[row_size]) {
		for (int i = 0; i < row_size; i++) {
			data[i] = src[i];
		}
	}

	Vec<T, col_size>& operator[](size_t row_index) {
		assert(row_index < row_size);
		return data[row_index];
	}
	const Vec<T, col_size> operator[](size_t row_index) const {
		assert(row_index < row_size);
		return data[row_index];
	}
	Vec<T, col_size> getCol(size_t col_index) const {
		Vec<T, col_size> ret;
		for (int i = 0; i < row_size; i++) {
			ret[i] = data[i][col_index];
		}
		return ret;
	}
	void setCol(size_t col_index, const Vec<T, row_size>& src) {
		for (int i = 0; i < row_size; i++) {
			data[i][col_index] = src[i];
		}
	}
	Mat<T, col_size, row_size> transpose() {
		Mat<T, col_size, row_size> ret;
		for (int i = 0; i < row_size; i++) {
			for (int j = 0; j < col_size; j++) {
				ret[j][i] = data[i][j];
			}
		}
		return ret;
	}

	template<size_t r_size, size_t c_size>
	Mat<T, r_size, c_size> get_minor() {
		assert(r_size <= row_size);
		assert(r_size <= col_size);
		Mat<T, r_size, c_size> ret;
		for (int i = 0; i < r_size; i++) {
			for (int j = 0; j < c_size; j++) {
				ret[i][j] = data[i][j];
			}
		}
		return ret;
	}

	static Mat<T, row_size, col_size> identity() {
		Mat<T, row_size, col_size> ret;
		for (int i = 0; i < row_size; i++) {
			for (int j = 0; j < col_size; j++) {
				ret[i][j] = T(i == j);
			}
		}
		return ret;
	}
};

template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator*(const T& lhs, const Mat<T, row_size, col_size>& rhs) {
	Mat<T, row_size, col_size> ret = rhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] *= lhs;
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator*(const Mat<T, row_size, col_size>& lhs, const T& rhs) {
	Mat<T, row_size, col_size> ret = lhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] *= rhs;
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator+(const T& lhs, const Mat<T, row_size, col_size>& rhs) {
	Mat<T, row_size, col_size> ret = rhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] += lhs;
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator+(const Mat<T, row_size, col_size>& lhs, const T& rhs) {
	Mat<T, row_size, col_size> ret = lhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] += rhs;
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator-(const Mat<T, row_size, col_size>& lhs, const T& rhs) {
	Mat<T, row_size, col_size> ret = lhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] -= rhs;
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator/(const Mat<T, row_size, col_size>& lhs, const T& rhs) {
	Mat<T, row_size, col_size> ret = lhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] /= rhs;
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator+(const Mat<T, row_size, col_size>& lhs, const Mat<T, row_size, col_size>& rhs) {
	Mat<T, row_size, col_size> ret = lhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] += rhs[i][j];
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Mat<T, row_size, col_size> operator-(const Mat<T, row_size, col_size>& lhs, const Mat<T, row_size, col_size>& rhs) {
	Mat<T, row_size, col_size> ret = lhs;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i][j] -= rhs[i][j];
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size_left, size_t col_size_right>
Mat<T, row_size, col_size_right> operator*(const Mat<T, row_size, col_size_left>& lhs, const Mat<T, col_size_left, col_size_right>& rhs) {
	Mat<T, row_size, col_size_right> ret;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size_right; j++) {
			ret[i][j] = 0;
			for (int k = 0; k < col_size_left; k++) {
				ret[i][j] += (lhs[i][k] * rhs[k][j]);
			}
		}
	}
	return ret;
}
template<typename T, size_t row_size, size_t col_size>
Vec<T, row_size> operator*(const Mat<T, row_size, col_size>& lhs, const Vec<T, col_size>& rhs) {
	Vec<T, row_size> ret;
	for (int i = 0; i < row_size; i++) {
		for (int j = 0; j < col_size; j++) {
			ret[i] += lhs[i][j] * rhs[j];
		}
	}
	return ret;
}

// TODO：待完成：余子式；伴随矩阵；逆转置矩阵；行列式

typedef Mat<int, 3, 3> mat3i;
typedef Mat<float, 3, 3> mat3f;
typedef Mat<int, 4, 4> mat4i;
typedef Mat<float, 4, 4> mat4f;
#pragma endregion


/*———————————————————————————————————运算函数——————————————————————————————————*/
#define PI 3.1415926

template<typename T>
T clamp(T num, T _min, T _max) {
	return num < _min ? _min : (num > _max ? _max : num);
}

template<typename T>
T saturate(T num) {
	return num < T(0) ? T(0) : (num > T(1) ? T(1) : num);
}

template<typename T>
T lerp(T left, T right, T factor) {
	return left + (right - left) * factor;
}

inline float to_radians(float angle) {
	return (angle * PI) / 180.f;
}