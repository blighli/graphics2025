#pragma once
#ifndef __SETTING_H__
#define __SETTING_H__

#include "Camera.h"
#include "Shape.h"
#include <random>
#include <vector>


//-------- windows settings-----------
extern const unsigned int SCR_WIDTH = 800;
extern const unsigned int SCR_HEIGHT = 800;

extern bool mouse = true;

//----------camera settings------------
extern Camera camera(glm::vec3(0.0f, 0.0f, 4.0f));
extern newCamera newcamera(90);
extern float lastX = SCR_HEIGHT / 2.0f;
extern float lastY = SCR_WIDTH / 2.0f;
extern bool firstMouse = true;
extern float deltaTime = 0.0f;
extern float lastFrame = 0.0f;

unsigned int frameCount = 0;

//-----------end------------------------

extern const float PI = 3.14159265359;

//----------Connle Box DATA-----------------
extern std::vector<glm::vec3> Left = {
	{-2,-2,-1},
	{-2, 2,-1},
	{-2, 2,-7},
	{-2,-2,-7}
};
extern std::vector<glm::vec3> Right = {
	{ 2,-2,-1},
	{ 2, 2,-1},
	{ 2, 2,-7},
	{ 2,-2,-7}
};

extern std::vector<glm::vec3> Up = {
	{ 2, 2,-1},
	{-2, 2,-1},
	{-2, 2,-7},
	{ 2, 2,-7}
};

extern std::vector<glm::vec3> Down = {
	{ 2,-2,-1},
	{-2,-2,-1},
	{-2,-2,-7},
	{ 2,-2,-7}
};

extern std::vector<glm::vec3> Back = {
	{-2,-2,-7},
	{-2, 2,-7},
	{ 2, 2,-7},
	{ 2,-2,-7}
};

extern std::vector<glm::vec3> LightUp = {
	{ 0.5f, 1.999f,-4},
	{-0.5f, 1.999f,-4},
	{-0.5f, 1.999f,-6},
	{ 0.5f, 1.999f,-6}
};

extern Material RED({ 1,0,0 });
extern Material GREEN({ 0,1,0 });  
extern Material BLUE({ 0,0,1 });   
extern Material YELLOW({ 1,1,0 }); 
extern Material PURPLE({ 1,0,1 }); 
extern Material CYAN({ 0,1,1 });   
extern Material WHITE({ 1,1,1 });  
extern Material GREY({ 0.8f,0.8f,0.8f });
extern Material WHITE_MIRROR({ 1,1,1 }, 0.05f, 1.0f, 0.0f);
extern Material WHITE_REFRAC({ 1,1,1 }, 0.05f, 1.0f, 0.0f, 1.25f, 1.9f);

#endif // !__SETTING_H__
