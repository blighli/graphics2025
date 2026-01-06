#include "scene_func.h"

Scene_Nanosuit::Scene_Nanosuit(){
    obj_shader = new Shader("./shader/objShader.vs", "./shader/objShader.fs");
    light_shader = new Shader("./shader/lightShader.vs", "./shader/lightShader.fs");

    string model_path = "./model/nanosuit/nanosuit.obj";
    mymodel = new Model(obj_shader, model_path.c_str());

    light_cube = new Cube(light_shader, NULL);	// light cube, just a point light source
    light_cube->shader->use();
	light_cube->shader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);	// TODO: this part just influence cube color, useless now

    direct_light = new DirectLight("directLight", vec3(0.05f, 0.05f, 0.05f), vec3(0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec3(-0.2f, -1.0f, -0.3f));

    // depth test setting
	glEnable(GL_DEPTH_TEST);
}

Scene_Nanosuit::~Scene_Nanosuit()
{
    delete obj_shader;
    delete light_shader;
    delete mymodel;
    delete light_cube;
    delete direct_light;
}

void Scene_Nanosuit::renderFunc(){
    // refresh screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // update camera view
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.ZoomScale), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    mymodel->shader->updateCameraView(view, projection);
    light_cube->shader->updateCameraView(view, projection);

    // flash light move with camera, so it need to be defined in render loop
    Light* flash_light = new SpotLight("spotLight", vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f), vec3(camera.Front),
                                    vec3(camera.Position), 1.0f, 0.09f, 0.032f, glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(15.0f)));



    /* paint light cube*/
    float time = (float)glfwGetTime();
    vec3 lightPos = vec3( glm::cos(time), 1.0f, glm::sin(time));
    Light* point_light1 = new PointLight("pointLights[0]", vec3(0.05f, 0.05f, 0.05f), vec3(0.8f, 0.8f, 0.8f), vec3(1.0f, 1.0f, 1.0f),
                                            lightPos, 1.0f, 0.09f, 0.032f);

    light_cube->shader->use();
    glm::mat4 model;
    model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
    light_cube->shader->setMat4("model", model);
    light_cube->Draw(36, GL_TRIANGLES);

    /* end paint */

    // draw model

    mymodel->shader->use();
    direct_light->applyLight(mymodel->shader);
    point_light1->applyLight(mymodel->shader);
    flash_light->applyLight(mymodel->shader);
    mymodel->shader->setVec3("viewPos", camera.Position);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.f, 0.0f)); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(0.2f));	// it's a bit too big for our scene, so scale it down
    glm::mat3 itModel = glm::mat3(transpose(inverse(model)));
    mymodel->shader->setMat4("model", model);
    mymodel->shader->setMat3("itModel", itModel);
    mymodel->Draw();

    // end model

    delete flash_light;
	delete point_light1;
}
