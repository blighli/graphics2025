#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "Mesh.h"

bool loadOBJ_PN_Fast(const std::string& path, std::vector<VertexPN>& outVerts, std::vector<uint32_t>& outIdx);
void centerAndScalePN(std::vector<VertexPN>& verts, float targetSize = 2.0f);
