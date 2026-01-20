#include "BVH.h"
#include <algorithm>
#include <iostream>
#include <string>

const float _MAX_ = 114514.0f;

using namespace glm;

int BuildBVH(std::vector<Triangle_encoded>& triangles, std::vector<BVHNode>& nodes, int left, int right, int n)
{
    if(left > right)
        return 0;
    
    nodes.push_back(BVHNode());
    int id = nodes.size()-1;
    nodes[id].AA = glm::vec3(114514.0f);
    nodes[id].BB = glm::vec3(-114514.0f);
    nodes[id].index = nodes[id].left = nodes[id].right = nodes[id].n = 0;

    for (int i = left; i <= right; i++)
    {
        // AA
        float minX = min(triangles[i].p1.x, min(triangles[i].p2.x, triangles[i].p3.x));
        float minY = min(triangles[i].p1.y, min(triangles[i].p2.y, triangles[i].p3.y));
        float minZ = min(triangles[i].p1.z, min(triangles[i].p2.z, triangles[i].p3.z));
        nodes[id].AA.x = min(nodes[id].AA.x, minX);
        nodes[id].AA.y = min(nodes[id].AA.y, minY);
        nodes[id].AA.z = min(nodes[id].AA.z, minZ);

        // BB
        float maxX = max(triangles[i].p1.x, max(triangles[i].p2.x, triangles[i].p3.x));
        float maxY = max(triangles[i].p1.y, max(triangles[i].p2.y, triangles[i].p3.y));
        float maxZ = max(triangles[i].p1.z, max(triangles[i].p2.z, triangles[i].p3.z));
        nodes[id].BB.x = max(nodes[id].BB.x, maxX);
        nodes[id].BB.y = max(nodes[id].BB.y, maxY);
        nodes[id].BB.z = max(nodes[id].BB.z, maxZ);
    }

    //std::cout << "right = " << right << "   left = " << left << "  n = " << n << std::endl;

    if ((right - left + 1) <= n)
    {
        nodes[id].n = right - left + 1;
        nodes[id].index = left;
        //std::cout << id << std::endl;
        return id;
    }

    // 划分
    // 先求最长轴
    float lenX = nodes[id].BB.x - nodes[id].AA.x;
    float lenY = nodes[id].BB.y - nodes[id].AA.y;
    float lenZ = nodes[id].BB.z - nodes[id].AA.z;

    // X
    if (lenX > lenY && lenX > lenZ)
        std::sort(triangles.begin() + left, triangles.begin() + right + 1,
            [](const Triangle_encoded& t1, const Triangle_encoded& t2) {
                vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / 3.0f;
                vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / 3.0f;

                return center1.x < center2.x;
            });
    // Y
    else if (lenY > lenX && lenY > lenZ)
        std::sort(triangles.begin() + left, triangles.begin() + right + 1,
            [](const Triangle_encoded& t1, const Triangle_encoded& t2) {
                vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / 3.0f;
                vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / 3.0f;

                return center1.y < center2.y;
            });
    // Z
    else 
        std::sort(triangles.begin() + left, triangles.begin() + right + 1,
            [](const Triangle_encoded& t1, const Triangle_encoded& t2) {
                vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / 3.0f;
                vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / 3.0f;

                return center1.z < center2.z;
            });

    
    int mid = (left + right) / 2;
    int l = BuildBVH(triangles, nodes, left, mid, n);
    int r = BuildBVH(triangles, nodes, mid + 1, right, n);

    nodes[id].left = l;
    nodes[id].right = r;

    return id;
}

void myPrint(glm::vec3 v, std::string s)
{
    std::cout << s << ":(" << v.x << "," << v.y << "," << v.z << ")" << std::endl;
}

BVHNode_encode encodeBVH(const BVHNode& bvh)
{
    BVHNode_encode res;

    res.children = glm::vec3(bvh.left, bvh.right, 0);
    res.information = glm::vec3(bvh.n, bvh.index, 0);
    res.AA = bvh.AA;
    res.BB = bvh.BB;

    if (false) {
        myPrint(res.children, "childs");
        myPrint(res.information, "info");
        myPrint(res.AA, "AA");
        myPrint(res.BB, "BB");
        std::cout << std::endl;
    }
    return res;
}
