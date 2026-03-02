#include "scene_func.h"

#include <vector>
using std::vector;

extern float deltaTime;

Scene_SolarSystem::Scene_SolarSystem(){

    // TODO: modify shader
    sun_shader = new Shader("./shader/simpleTextureShader.vs", "./shader/simpleTextureShader.fs");
    obj_shader = new Shader("./shader/planetShader.vs", "./shader/planetShader.fs");


    Texture* earth_diffuse = loadTexture("diffuse0", "./texture/earthmap1k.jpg");
    Texture* earth_specular = loadTexture("specular0", "./texture/earthspec1k.jpg");
    Texture* moon_diffuse = loadTexture("diffuse0", "./texture/moonmap1k.jpg");
    Texture* sunmap = loadTexture("diffuse0", "./texture/sunmap.jpg");

    vector<Texture*> earth_texture = {earth_diffuse, earth_specular};
    vector<Texture*> moon_texture = {moon_diffuse};
    vector<Texture*> sun_texture = {sunmap};

    Material* earth_material = new PhongMaterial("material", earth_texture);
    Material* moon_material = new PhongMaterial("material", moon_texture);
    Material* sun_material = new PhongMaterial("material", sun_texture);

    earth = new Sphere(obj_shader, earth_material, 0.2f);
    moon = new Sphere(obj_shader, moon_material, 0.05f);
    sun = new Sphere(sun_shader, sun_material);

    earth_autorotation_rate = 6.0f;
    moon_autorotation_rate = 6.0f;
    sun_autorotation_rate = 2.0f;

    earth_revolution_speed = 3.0f;
    moon_revolution_speed = 10.0f;

    earth_orbit_radius = 2.5f;
    moon_orbit_radius = 0.3f;

    earth_revolution_angle = 0.f;
    earth_rotation_angle = 0.f;
    moon_revolution_angle = 0.f;
    moon_rotation_angle = 0.f;
    sun_rotation_angle = 0.f;

    // depth test setting
	glEnable(GL_DEPTH_TEST);
}


Scene_SolarSystem::~Scene_SolarSystem(){
    delete skybox_shader;
    delete obj_shader;
    delete earth;
    delete moon;
    delete sun;
}


void Scene_SolarSystem::renderFunc(){
    // refresh screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // update camera view
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.ZoomScale), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    earth->shader->updateCameraView(view, projection);
    moon->shader->updateCameraView(view, projection);
    sun->shader->updateCameraView(view, projection);

    Light* point_light1 = new PointLight("pointLights[0]", vec3(0.0f, 0.0f, 0.0f), vec3(0.8f, 0.8f, 0.8f), vec3(1.0f, 1.0f, 1.0f),
                                            vec3(0.0f, 0.0f, 0.0f), 1.0f, 0.09f, 0.032f);

    earth_revolution_angle += deltaTime * earth_revolution_speed;
    earth_rotation_angle += deltaTime * earth_autorotation_rate;
    moon_revolution_angle += deltaTime * moon_revolution_speed;
    moon_rotation_angle += deltaTime * moon_autorotation_rate;
    sun_rotation_angle += deltaTime * sun_autorotation_rate;

    // sun
    sun->shader->use();
    glm::mat4 sun_model = glm::mat4(1.0f);
    sun_model = glm::translate(sun_model, glm::vec3(0.0f, 0.0f, 0.0f));
    sun_model = glm::rotate(sun_model, glm::radians(sun_rotation_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    sun->shader->setMat4("model", sun_model);
    sun->Draw();

    // earth
    earth->shader->use();
    point_light1->applyLight(earth->shader);
    earth->shader->setVec3("viewPos", camera.Position);
    glm::mat4 earth_model = glm::mat4(1.0f);
    earth_model = glm::rotate(earth_model, glm::radians(earth_revolution_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    earth_model = glm::translate(earth_model, glm::vec3(earth_orbit_radius, 0.0f, 0.0f));
    earth_model = glm::rotate(earth_model, glm::radians(earth_rotation_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    earth_model = glm::rotate(earth_model, glm::radians(23.5f), glm::vec3(0.0f, 0.0f, 1.0f));
    earth_model = glm::rotate(earth_model, glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat3 earth_itmodel = glm::mat3(transpose(inverse(earth_model)));
    earth->shader->setMat4("model", earth_model);
    earth->shader->setMat3("itModel", earth_itmodel);
    earth->Draw();

    // moon
    moon->shader->use();
    point_light1->applyLight(moon->shader);
    moon->shader->setVec3("viewPos", camera.Position);
    glm::mat4 moon_model = glm::mat4(1.0f);
    moon_model = glm::rotate(moon_model, glm::radians(earth_revolution_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    moon_model = glm::translate(moon_model, glm::vec3(earth_orbit_radius, 0.0f, 0.0f));
    moon_model = glm::rotate(moon_model, glm::radians(moon_revolution_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    moon_model = glm::translate(moon_model, glm::vec3(moon_orbit_radius, 0.0f, 0.0f));
    moon_model = glm::rotate(moon_model, glm::radians(moon_rotation_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat3 moon_itmodel = glm::mat3(transpose(inverse(moon_model)));
    moon->shader->setMat4("model", moon_model);
    moon->shader->setMat3("itModel", moon_itmodel);
    moon->Draw();

    delete point_light1;
}
