/*
 * Copyright (c) 2023-2025, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2023-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "shaderio.h"

[[vk::push_constant]]
ConstantBuffer<PushConstant> pushConst;
[[vk::binding(0)]]
ConstantBuffer<FrameInfo> frameInfo;

// Prepare payload to pass to mesh shader
groupshared TaskPayload taskPayload;

// Grass blade configuration
static const uint GRASS_SEGMENTS = 4;                         // Number of segments per grass blade
static const uint VERTICES_PER_GRASS = (GRASS_SEGMENTS + 1) * 2;  // Vertices per grass blade (strip)
static const uint TRIANGLES_PER_GRASS = GRASS_SEGMENTS * 2;   // Triangles per grass blade
static const uint GRASS_BLADES_PER_MESH = 8;                  // Grass blades per mesh workgroup

// Output from mesh shader to fragment shader
struct MeshOutput
{
  float4 position : SV_Position;
  float3 color : COLOR;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD0;
};

// Simple hash function for pseudo-random values
float hash(float2 p)
{
  return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

// 2D noise function for wind
float noise(float2 p)
{
  float2 i = floor(p);
  float2 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  
  float a = hash(i);
  float b = hash(i + float2(1.0, 0.0));
  float c = hash(i + float2(0.0, 1.0));
  float d = hash(i + float2(1.0, 1.0));
  
  return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

// Fractal Brownian Motion for terrain - creates natural looking hills
float fbm(float2 p)
{
  float value = 0.0;
  float amplitude = 0.5;
  float frequency = 1.0;
  
  // 4 octaves of noise
  for(int i = 0; i < 4; i++)
  {
    value += amplitude * noise(p * frequency);
    amplitude *= 0.5;
    frequency *= 2.0;
  }
  return value;
}

// Calculate terrain height at a world position
float getTerrainHeight(float2 worldPos)
{
  // Large scale hills
  float largeHills = fbm(worldPos * 0.02) * 8.0;
  
  // Medium bumps
  float mediumBumps = fbm(worldPos * 0.08) * 2.0;
  
  // Small details
  float smallDetails = fbm(worldPos * 0.25) * 0.5;
  
  return largeHills + mediumBumps + smallDetails - 5.0;  // Offset to center around y=0
}

// Calculate grass blade height variation based on position
float getGrassHeightMultiplier(float2 worldPos)
{
  // Base variation using noise
  float variation = fbm(worldPos * 0.1 + float2(100.0, 100.0));
  
  // Taller grass in lower areas (valleys), shorter on hilltops
  float terrainH = getTerrainHeight(worldPos);
  float terrainInfluence = saturate(1.0 - terrainH * 0.05);  // Lower terrain = taller grass
  
  // Combine: range from 0.4 to 1.4
  return 0.5 + variation * 0.6 + terrainInfluence * 0.3;
}

// Calculate wind displacement for grass - smooth and natural with position-based variation
float2 calculateWind(float2 worldPos, float time, float height, float swayStrength)
{
  // Base wind strength varies by position using noise-like pattern
  float posNoise = sin(worldPos.x * 0.15) * cos(worldPos.y * 0.12) * 0.5 + 0.5;  // 0 to 1
  float windStrength = (0.15 + posNoise * 0.25) * swayStrength;  // Range scaled by swayStrength
  
  // Add turbulence zones - some areas have stronger gusts
  float gustZone = sin(worldPos.x * 0.03 + time * 0.5) * sin(worldPos.y * 0.04 + time * 0.3);
  windStrength += max(0.0, gustZone) * 0.2 * swayStrength;
  
  // Primary wind wave - large slow movement (phase varies by position)
  float phase1 = worldPos.x * 0.05 + worldPos.y * 0.03;
  float wave1 = sin(time * 1.5 + phase1) * 0.6;
  
  // Secondary wind wave - medium frequency with position-based amplitude
  float phase2 = worldPos.x * 0.12 + worldPos.y * 0.08;
  float amp2 = 0.2 + sin(worldPos.x * 0.08) * 0.15;  // Amplitude varies spatially
  float wave2 = sin(time * 2.3 + phase2) * amp2;
  
  // Tertiary wave - small fast flutter with local variation
  float phase3 = worldPos.x * 0.25 + worldPos.y * 0.18;
  float amp3 = 0.05 + cos(worldPos.y * 0.1) * 0.08;
  float wave3 = sin(time * 4.0 + phase3) * amp3;
  
  float combinedWave = wave1 + wave2 + wave3;
  
  // 使用 pushConst.windDirection 作为风向（可由UI控制）
  float2 windDir = length(pushConst.windDirection) > 0.001 ? normalize(pushConst.windDirection) : float2(1.0, 0.3);
  
  // Wind effect increases with height squared (grass bends more at top)
  float heightFactor = height * height;
  
  return windDir * combinedWave * windStrength * heightFactor;
}

// Test if a sphere (center + radius) is inside frustum
// Returns true if visible (inside or intersecting frustum)
bool isSphereInFrustum(float3 center, float radius)
{
  for(int i = 0; i < 6; i++)
  {
    float3 planeNormal   = frameInfo.frustumPlanes[i].xyz;
    float  planeDistance = frameInfo.frustumPlanes[i].w;

    // Distance from plane to sphere center
    float distance = dot(planeNormal, center) + planeDistance;

    // If sphere is completely outside any plane, it's not visible
    if(distance < -radius)
    {
      return false;
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------
// Task Shader - Per-grass-patch GPU culling with compaction
// Each thread tests one grass patch against the frustum (32 threads test 32 patches)
//--------------------------------------------------------------------------------------------------
[shader("amplification")]
[numthreads(TASKSHADER_WORKGROUP_SIZE, 1, 1)]
void taskMain(uint3 groupThreadID: SV_GroupThreadID, uint3 groupID: SV_GroupID)
{
  uint threadID = groupThreadID.x;  // 0 to 31

  // Get grid position for this task shader workgroup
  uint gridX = groupID.x;
  uint gridZ = groupID.y;

  // Check if this workgroup is within bounds
  uint startPatchX = gridX * BOXES_PER_TASK;
  if(startPatchX >= pushConst.totalBoxesX || gridZ >= pushConst.totalBoxesZ)
  {
    return;  // Outside grid bounds
  }


  // Pass grid position to mesh shader via payload
  if(threadID == 0)
  {
    taskPayload.gridX = gridX;
    taskPayload.gridZ = gridZ;
  }
  GroupMemoryBarrierWithGroupSync();  // Ensure payload is initialized before all threads use it

  // Calculate how many patches this workgroup will actually test
  uint patchesInThisTile = min(BOXES_PER_TASK, pushConst.totalBoxesX - startPatchX);

  // Each thread tests one grass patch
  uint localPatchIndex = threadID;
  bool patchSurvives   = false;

  if(localPatchIndex < patchesInThisTile)
  {
    // Calculate global patch position
    uint globalPatchX = startPatchX + localPatchIndex;

    // Calculate patch center position in world space
    float xOffset = (float(globalPatchX) - float(pushConst.totalBoxesX - 1) * 0.5) * pushConst.spacing;
    float zOffset = (float(gridZ) - float(pushConst.totalBoxesZ - 1) * 0.5) * pushConst.spacing;

    // Get terrain height at this position
    float terrainY = getTerrainHeight(float2(xOffset, zOffset));
    float3 patchCenter = float3(xOffset, terrainY, zOffset);

    // Bounding sphere radius for grass blade (height-based, account for terrain variation)
    float grassHeight = pushConst.boxSize * 2.0 * 1.5;  // Max possible height with variation
    float boundingRadius = grassHeight * 1.5 + 5.0;     // Extra margin for terrain height variation

    // Test if this grass patch is visible
    patchSurvives = isSphereInFrustum(patchCenter + float3(0, grassHeight * 0.5, 0), boundingRadius);
  }

  if(patchSurvives)
  {
    // Compact surviving patches into a contiguous array with no gaps.
    uint offset                             = WavePrefixCountBits(patchSurvives);
    taskPayload.survivingBoxIndices[offset] = uint8_t(localPatchIndex);
  }

  // Count total number of surviving patches across the entire wave.
  uint numSurvive = WaveActiveCountBits(patchSurvives);

  // Store total count of surviving patches.
  if(threadID == 0)
  {
    taskPayload.numSurvivingBoxes = numSurvive;
    // Atomically add the number of surviving patches to the global counter
    Statistics* stats = (Statistics*)(pushConst.statisticsAddr);
    InterlockedAdd(stats->boxesDrawn, numSurvive);
  }

  // Emit mesh shader workgroups to process surviving grass patches
  if(threadID == 0 && taskPayload.numSurvivingBoxes > 0)
  {
    uint numMeshWorkgroups = (taskPayload.numSurvivingBoxes + GRASS_BLADES_PER_MESH - 1) / GRASS_BLADES_PER_MESH;
    DispatchMesh(numMeshWorkgroups, 1, 1, taskPayload);
  }
}

//--------------------------------------------------------------------------------------------------
// Mesh Shader - generates grass blades as triangle strips with wind animation
// Each grass blade is rendered as a tapered quad strip for realistic appearance
//--------------------------------------------------------------------------------------------------
// 修正：只保留合法 Slang meshMain 定义
[shader("mesh")]
[outputtopology("triangle")]
[numthreads(MESHSHADER_WORKGROUP_SIZE, 1, 1)]
void meshMain(uint3 groupThreadID: SV_GroupThreadID,
        uint3 groupID: SV_GroupID,
        OutputVertices<MeshOutput, GRASS_BLADES_PER_MESH * VERTICES_PER_GRASS> verts,
        OutputIndices<uint3, GRASS_BLADES_PER_MESH * TRIANGLES_PER_GRASS> indices)
{
  uint threadID        = groupThreadID.x;
  uint meshWorkgroupID = groupID.x;

  // Get grid position from task shader payload
  uint gridX = taskPayload.gridX;
  uint gridZ = taskPayload.gridZ;

  // Each mesh workgroup processes up to GRASS_BLADES_PER_MESH grass blades
  uint baseBladeOffset = meshWorkgroupID * GRASS_BLADES_PER_MESH;

  // Calculate how many grass blades this mesh workgroup should render
  uint numBlades = min(GRASS_BLADES_PER_MESH, taskPayload.numSurvivingBoxes - baseBladeOffset);

  // Calculate output counts
  uint totalVertices   = numBlades * VERTICES_PER_GRASS;
  uint totalPrimitives = numBlades * TRIANGLES_PER_GRASS;

  float grassHeight = pushConst.boxSize * 2.0;
  float grassWidth = pushConst.boxSize * 0.15;
  float spacing = pushConst.spacing;

  // Set primitive count
  SetMeshOutputCounts(totalVertices, totalPrimitives);

  uint startPatchX = gridX * BOXES_PER_TASK;

  // 取模，避免 float 精度丢失导致 hash/扰动异常
  // 1024 可根据实际需求调整，保证 hash 输入不大于 float 精度
  const uint HASH_MOD = 1024;

  // Distribute vertex work across all threads
  for(uint vertexIndex = threadID; vertexIndex < totalVertices; vertexIndex += MESHSHADER_WORKGROUP_SIZE)
  {
    uint bladeIndex = vertexIndex / VERTICES_PER_GRASS;
    uint localVertexIndex = vertexIndex % VERTICES_PER_GRASS;
        
    uint segmentIndex = localVertexIndex / 2;
    uint side = localVertexIndex % 2;  // 0 = left, 1 = right

    // Get the local blade index from the surviving list
    uint localPatchIndex = taskPayload.survivingBoxIndices[baseBladeOffset + bladeIndex];
    uint globalPatchX = startPatchX + localPatchIndex;

    // 取模后用于 hash/扰动/颜色
    uint safePatchX = globalPatchX % HASH_MOD;
    uint safeGridZ = gridZ % HASH_MOD;

    // Base grid position
    float baseX = (float(globalPatchX) - float(pushConst.totalBoxesX - 1) * 0.5f) * spacing;
    float baseZ = (float(gridZ) - float(pushConst.totalBoxesZ - 1) * 0.5f) * spacing;

    // Strong randomization to break grid pattern - random position within cell
    // Use multiple hash values for better distribution
    float rand1 = hash(float2(safePatchX * 1.0, safeGridZ * 1.0));
    float rand2 = hash(float2(safePatchX * 2.7, safeGridZ * 3.1));
    float rand3 = hash(float2(safeGridZ * 5.3, safePatchX * 7.9));
    float rand4 = hash(float2(safeGridZ * 11.3, safePatchX * 13.7));
        
    // Random offset covers full cell (-0.5 to +0.5 of spacing)
    float randX = (rand1 + rand2 * 0.5) / 1.5 - 0.5;  // Range: -0.5 to 0.5
    float randZ = (rand3 + rand4 * 0.5) / 1.5 - 0.5;  // Range: -0.5 to 0.5
        
    float xOffset = baseX + randX * spacing;
    float zOffset = baseZ + randZ * spacing;

    // Calculate terrain height at this position (ground level varies)
    float terrainY = getTerrainHeight(float2(xOffset, zOffset));

    // Grass height varies based on position (using noise + terrain influence)
    float heightMultiplier = getGrassHeightMultiplier(float2(xOffset, zOffset));
    float bladeHeight = grassHeight * heightMultiplier;

    // Random rotation for each blade
    float rotation = hash(float2(safePatchX * 13.7, safeGridZ * 17.3)) * 3.14159 * 2.0;

    // Calculate height factor (0 at base, 1 at top)
    float t = float(segmentIndex) / float(GRASS_SEGMENTS);
    float y = t * bladeHeight;

    // Width tapers toward top
    float currentWidth = grassWidth * (1.0 - t * 0.85);

    // Calculate wind displacement with sway strength from push constants
    float2 windOffset = calculateWind(float2(xOffset, zOffset), pushConst.time * pushConst.animSpeed, t, pushConst.swayStrength);

    // Calculate vertex position
    float sideOffset = (side == 0) ? -currentWidth : currentWidth;
        
    // Rotate the blade
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    float3 localPos = float3(sideOffset * cosR, y, sideOffset * sinR);
        
    // Apply wind (increases with height)
    localPos.x += windOffset.x * bladeHeight;
    localPos.z += windOffset.y * bladeHeight;

    // World position with terrain height applied
    float3 worldPos = float3(xOffset, terrainY, zOffset) + localPos;

    float4 clipPos = mul(mul(float4(worldPos, 1.0f), frameInfo.view), frameInfo.proj);

    // Grass color: smooth gradient from base to tip, no per-blade variation
    float3 baseColor = float3(0.08, 0.22, 0.04); // 深绿色（根部）
    float3 tipColor  = float3(0.35, 0.65, 0.18); // 浅绿色（顶端）
    float3 color = lerp(baseColor, tipColor, t); // 按高度平滑过渡

    // Simple normal (facing outward from blade center)
    float3 normal = normalize(float3(cosR, 0.3, sinR));

    verts[vertexIndex].position = clipPos;
    verts[vertexIndex].color = color;
    verts[vertexIndex].normal = normal;
    verts[vertexIndex].uv = float2(float(side), t);
  }

  // Distribute primitive work across all threads - generate triangles for quad strips
  for(uint primitiveIndex = threadID; primitiveIndex < totalPrimitives; primitiveIndex += MESHSHADER_WORKGROUP_SIZE)
  {
    uint bladeIndex = primitiveIndex / TRIANGLES_PER_GRASS;
    uint triIndex = primitiveIndex % TRIANGLES_PER_GRASS;
    uint baseVertex = bladeIndex * VERTICES_PER_GRASS;

    uint segmentIndex = triIndex / 2;
    uint triInSegment = triIndex % 2;

    // Create quad from two triangles
    uint v0 = baseVertex + segmentIndex * 2;
    uint v1 = v0 + 1;
    uint v2 = v0 + 2;
    uint v3 = v0 + 3;

    if(triInSegment == 0)
    {
      indices[primitiveIndex] = uint3(v0, v1, v2);
    }
    else
    {
      indices[primitiveIndex] = uint3(v1, v3, v2);
    }
  }
}

//--------------------------------------------------------------------------------------------------
// Fragment Shader - grass shading with simple lighting
//--------------------------------------------------------------------------------------------------
[shader("pixel")]
float4 fragmentMain(MeshOutput input)
    : SV_Target
{
  // Simple directional light from above-right
  float3 lightDir = normalize(float3(0.3, 1.0, 0.2));
  float NdotL = max(dot(input.normal, lightDir), 0.0);
  
  // Ambient + diffuse lighting
  float3 ambient = input.color * 0.4;
  float3 diffuse = input.color * NdotL * 0.6;
  
  // Add slight subsurface scattering effect for grass
  float3 finalColor = ambient + diffuse;
  
  // Brighten tips slightly
  finalColor = lerp(finalColor, finalColor * 1.2, input.uv.y);
  
  return float4(finalColor, 1.0f);
}
