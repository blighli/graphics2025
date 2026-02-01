#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include "maths.hpp"
#include "tgaimage.hpp"

class Model {
private:
    std::vector<vec3f> verts_;
    std::vector<std::vector<vec3i> > faces_; // attention, this Vec3i means vertex/uv/normal
    std::vector<vec3f> norms_;
    std::vector<vec2f> uv_;
    TGAImage diffusemap_;
    TGAImage normalmap_;
    TGAImage specularmap_;
    void load_texture(std::string filename, const char *suffix, TGAImage &img);
public:
    Model(const char *filename);
    ~Model();
    int nverts();
    int nfaces();
    vec3f normal(int iface, int nthvert);
    vec3f normal(vec2f uv);
    vec3f vert(int i);
    vec3f vert(int iface, int nthvert);
    vec2f uv(int iface, int nthvert);
    TGAColor diffuse(vec2f uv);
    float specular(vec2f uv);
    std::vector<int> face(int idx);

    TGAImage* get_diffuse_map();
    TGAImage* get_specular_map();
    TGAImage* get_normal_map();
};
#endif //__MODEL_H__
