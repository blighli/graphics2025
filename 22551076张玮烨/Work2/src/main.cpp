#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------------------- Shader --------------------
const char* vertexShaderSource = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTex;

out vec3 FragPos;
out vec3 Normal;
out vec2 Tex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Tex = aTex;
    gl_Position = projection * view * vec4(FragPos, 1.0);
})";

const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 Tex;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    vec3 ambient = vec3(0.2);
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.5);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * vec3(1.0);
    vec3 lighting = ambient + diffuse + specular;
    vec3 texColor = texture(texture1, Tex).rgb;
    FragColor = vec4(lighting * texColor, 1.0);
})";

// -------------------- Sphere --------------------
void CreateSphere(float radius, int sectorCount, int stackCount,
                  std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    float x, y, z, nx, ny, nz, s, t;
    float pi = M_PI;
    float sectorStep = 2 * pi / sectorCount;
    float stackStep = pi / stackCount;

    for(int i = 0; i <= stackCount; ++i) {
        float stackAngle = pi/2 - i*stackStep;
        float xy = radius * cos(stackAngle);
        z = radius * sin(stackAngle);

        for(int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * sectorStep;
            x = xy * cos(sectorAngle);
            y = xy * sin(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            nx = x / radius; ny = y / radius; nz = z / radius;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
            s = (float)j / sectorCount; t = (float)i / stackCount;
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    for(int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;
        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if(i != 0) { indices.push_back(k1); indices.push_back(k2); indices.push_back(k1+1); }
            if(i != (stackCount-1)) { indices.push_back(k1+1); indices.push_back(k2); indices.push_back(k2+1); }
        }
    }
}

// -------------------- Texture --------------------
unsigned int LoadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if(data) {
        GLenum format = (nrChannels == 3 ? GL_RGB : GL_RGBA);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

// -------------------- Hover --------------------
struct SphereInfo {
    const char* name;
    float screenX;
    float screenY;
    float radius;
};
SphereInfo spheres[3];

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    ypos = 600 - ypos; // 仅翻转 Y
    const char* hoverName = nullptr;
    for(int i=0;i<3;++i){
        float dx = xpos - spheres[i].screenX;
        float dy = ypos - spheres[i].screenY;
        if(dx*dx + dy*dy <= spheres[i].radius*spheres[i].radius){
            hoverName = spheres[i].name;
            break;
        }
    }
    if(hoverName){
        std::string title = std::string("当前为: ") + hoverName;
        glfwSetWindowTitle(window, title.c_str());
    } else {
        glfwSetWindowTitle(window, "22551076-张玮烨-简单太阳系");
    }
}

// -------------------- Project --------------------
auto ProjectToScreenGLM = [](const glm::vec3& pos,
                             const glm::mat4& model,
                             const glm::mat4& view,
                             const glm::mat4& projection,
                             int windowWidth, int windowHeight) -> std::pair<float,float>
{
    glm::vec4 clipPos = projection * view * model * glm::vec4(pos,1.0f);
    if(clipPos.w <= 0.0001f) return {-1.0f,-1.0f};
    glm::vec3 ndc = glm::vec3(clipPos)/clipPos.w;
    if(ndc.x<-1.0f||ndc.x>1.0f||ndc.y<-1.0f||ndc.y>1.0f) return {-1.0f,-1.0f};
    float screenX = (ndc.x*0.5f+0.5f)*windowWidth;
    float screenY = (ndc.y*0.5f+0.5f)*windowHeight;
    return {screenX, screenY};
};

// -------------------- Main --------------------
int main() {
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800,600,"22551076-张玮烨-简单太阳系",NULL,NULL);
    if(!window){ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, CursorPosCallback);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout<<"Failed to initialize GLAD"<<std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertexShaderSource,NULL);
    glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragmentShaderSource,NULL);
    glCompileShader(fragmentShader);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vertexShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 球体数据
    std::vector<float> sunVerts,earthVerts,moonVerts;
    std::vector<unsigned int> sunInd,earthInd,moonInd;
    CreateSphere(1.0f,64,64,sunVerts,sunInd);
    CreateSphere(0.5f,64,64,earthVerts,earthInd);
    CreateSphere(0.15f,64,64,moonVerts,moonInd);

    unsigned int VAO,VBO,EBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);

    unsigned int texSun = LoadTexture("texture/2k_sun.jpg");
    unsigned int texEarth = LoadTexture("texture/2k_earth_daymap.jpg");
    unsigned int texMoon = LoadTexture("texture/2k_moon.jpg");

    float earthAngle=0.0f, moonAngle=0.0f;
    glm::vec3 cameraPos(0.0f,5.0f,10.0f);

    while(!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glm::mat4 view  = glm::lookAt(cameraPos, glm::vec3(0,0,0), glm::vec3(0,1,0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram,"lightPos"),1,glm::value_ptr(glm::vec3(0.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram,"viewPos"),1,glm::value_ptr(cameraPos));

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER,VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);

        // -------------------- 绘制太阳 --------------------
        glBufferData(GL_ARRAY_BUFFER,sunVerts.size()*sizeof(float),&sunVerts[0],GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,sunInd.size()*sizeof(unsigned int),&sunInd[0],GL_STATIC_DRAW);
        glm::mat4 sunmodel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,glm::value_ptr(sunmodel));
        glBindTexture(GL_TEXTURE_2D, texSun);
        glDrawElements(GL_TRIANGLES, sunInd.size(), GL_UNSIGNED_INT, 0);

        // -------------------- 绘制地球 --------------------
        earthAngle += 0.001f;
        glm::vec3 earthPos = glm::vec3(3.0f*cos(earthAngle),0.0f,3.0f*sin(earthAngle));
        glBufferData(GL_ARRAY_BUFFER,earthVerts.size()*sizeof(float),&earthVerts[0],GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,earthInd.size()*sizeof(unsigned int),&earthInd[0],GL_STATIC_DRAW);
        glm::mat4 earthmodel = glm::translate(glm::mat4(1.0f), earthPos);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,glm::value_ptr(earthmodel));
        glBindTexture(GL_TEXTURE_2D, texEarth);
        glDrawElements(GL_TRIANGLES, earthInd.size(), GL_UNSIGNED_INT, 0);

        // -------------------- 绘制月球 --------------------
        moonAngle += 0.005f;
        glm::vec3 moonPos = earthPos + glm::vec3(0.7f*cos(moonAngle),0.0f,1.5f*sin(moonAngle));
        glBufferData(GL_ARRAY_BUFFER,moonVerts.size()*sizeof(float),&moonVerts[0],GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,moonInd.size()*sizeof(unsigned int),&moonInd[0],GL_STATIC_DRAW);
        glm::mat4 moonmodel = glm::translate(glm::mat4(1.0f), moonPos);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,glm::value_ptr(moonmodel));
        glBindTexture(GL_TEXTURE_2D, texMoon);
        glDrawElements(GL_TRIANGLES, moonInd.size(), GL_UNSIGNED_INT, 0);

        // -------------------- 更新屏幕坐标 --------------------
        // 更新屏幕坐标
        spheres[0] = {"太阳",
            ProjectToScreenGLM(glm::vec3(0,0,0), sunmodel, view, projection,800,600).first,
            ProjectToScreenGLM(glm::vec3(0,0,0), sunmodel, view, projection,800,600).second,
            50.0f};

        spheres[1] = {"地球",
            ProjectToScreenGLM(glm::vec3(0,0,0), earthmodel, view, projection,800,600).first,
            ProjectToScreenGLM(glm::vec3(0,0,0), earthmodel, view, projection,800,600).second,
            30.0f};

        spheres[2] = {"月球",
            ProjectToScreenGLM(glm::vec3(0,0,0), moonmodel, view, projection,800,600).first,
            ProjectToScreenGLM(glm::vec3(0,0,0), moonmodel, view, projection,800,600).second,
            20.0f};

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
