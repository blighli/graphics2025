#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"


class material;

//记录碰撞点的信息
class hit_record {
  public:
    point3 p; //该点的坐标
    vec3 normal; //碰撞点的法线信息
    shared_ptr<material> mat; //碰撞点的材质信息
    double t; //射线在碰撞点的t值  at(t)
    bool front_face; //是否是碰撞的正面

    void set_face_normal(const ray& r,const vec3& outward_normal)
    {
      front_face = dot(r.direction(),outward_normal) < 0;
      normal = front_face ? outward_normal : -outward_normal;
    }

};

class hittable {
  public:
    virtual ~hittable() = default;
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

#endif