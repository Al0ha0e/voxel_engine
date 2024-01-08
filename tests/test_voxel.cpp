#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vengine.hpp>
#include <gameobject.hpp>
#include <component/camera.hpp>
#include <component/renderable_object.hpp>

const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;
const uint32_t levelnum = 5;
const float editR = 3;

vxe_common::Engine *engine;
float time_prev, time_delta;
vxe_terrain::EditInfo editInfo;
vke_common::TransformParameter camParam(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, glm::radians(0.0f), 0.0f));
vke_common::GameObject cameraGameObj(camParam);

void processInput(GLFWwindow *window, vke_common::GameObject *target, vke_common::GameObject *target2);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

int main()
{
    editInfo.info1.x = 0;
    engine = vxe_common::Engine::Init(WIDTH, HEIGHT, levelnum);
    vke_render::RenderResourceManager *manager = vke_render::RenderResourceManager::GetInstance();

    vke_common::TransformParameter targetParam(glm::vec3(-0.5f, 0.5f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    vke_common::GameObject targetGameObj(targetParam);
    vke_render::Material *material = manager->LoadMaterial("");
    vke_render::Mesh *mesh = manager->LoadMesh("");
    vke_component::RenderableObject renderObj(material, mesh, &targetGameObj);
    targetGameObj.components.push_back(&renderObj);

    vke_component::Camera camera(105, WIDTH, HEIGHT, 0.01, 100000, &cameraGameObj);
    cameraGameObj.components.push_back(&camera);
    vxe_terrain::Terrain::SetObserver(&cameraGameObj);

    // vke_render::RenderResourceManager *manager = vke_render::RenderResourceManager::GetInstance();
    GLFWwindow *window = engine->engine->environment->window;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float now = glfwGetTime();
        if (time_prev == 0)
            time_prev = glfwGetTime();
        time_delta = now - time_prev;
        time_prev = now;
        // std::cout << 1 / time_delta << std::endl;

        processInput(window, &cameraGameObj, &targetGameObj);

        engine->Update();
    }
    vkDeviceWaitIdle(engine->engine->environment->logicalDevice);

    // engine->MainLoop();

    vke_common::Engine::Dispose();
    return 0;
}

#define CHECK_KEY(x) if (glfwGetKey(window, x) == GLFW_PRESS)

float moveSpeed = 4.0f;
float rotateSpeed = 1.0f;

void processInput(GLFWwindow *window, vke_common::GameObject *target, vke_common::GameObject *target2)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    CHECK_KEY(GLFW_KEY_W)
    {
        target->TranslateLocal(glm::vec3(0, 0, -moveSpeed * time_delta));
    }
    CHECK_KEY(GLFW_KEY_A)
    {
        target->TranslateLocal(glm::vec3(-moveSpeed * time_delta, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_S)
    {
        target->TranslateLocal(glm::vec3(0, 0, moveSpeed * time_delta));
    }
    CHECK_KEY(GLFW_KEY_D)
    {
        target->TranslateLocal(glm::vec3(moveSpeed * time_delta, 0, 0));
    }

    CHECK_KEY(GLFW_KEY_UP)
    {
        target2->TranslateLocal(glm::vec3(0, -moveSpeed * time_delta * 0.5, 0));
    }
    CHECK_KEY(GLFW_KEY_LEFT)
    {
        target2->TranslateLocal(glm::vec3(-moveSpeed * time_delta * 0.5, 0, 0));
    }
    CHECK_KEY(GLFW_KEY_DOWN)
    {
        target2->TranslateLocal(glm::vec3(0, moveSpeed * time_delta * 0.5, 0));
    }
    CHECK_KEY(GLFW_KEY_RIGHT)
    {
        target2->TranslateLocal(glm::vec3(moveSpeed * time_delta * 0.5, 0, 0));
    }

    CHECK_KEY(GLFW_KEY_ENTER)
    {
        editInfo.info0 = glm::vec4(target2->transform.position, editR);
        vxe_terrain::Terrain::EditTerrain(vxe_terrain::TerrainEditor::EDIT_DEC, editInfo);
    }

    CHECK_KEY(GLFW_KEY_TAB)
    {
        editInfo.info0 = glm::vec4(target2->transform.position, editR);
        vxe_terrain::Terrain::EditTerrain(vxe_terrain::TerrainEditor::EDIT_INC, editInfo);
    }

    CHECK_KEY(GLFW_KEY_1)
    {
        editInfo.info1.x = 0;
    }

    CHECK_KEY(GLFW_KEY_2)
    {
        editInfo.info1.x = 1;
    }
    CHECK_KEY(GLFW_KEY_3)
    {
        editInfo.info1.x = 2;
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    static bool firstMouse = true;
    static float lastX, lastY;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    cameraGameObj.RotateGlobal(-xoffset * rotateSpeed * time_delta, glm::vec3(0.0f, 1.0f, 0.0f));
    cameraGameObj.RotateLocal(yoffset * rotateSpeed * time_delta, glm::vec3(1.0f, 0.0f, 0.0f));
}