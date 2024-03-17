#include <iostream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "instructions.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLFWwindow* window;
bool setGLFW()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1, 1, "MegaBoy", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const int width = { static_cast<int>(mode->width * 0.65f) };
    const int height = width / 2;

    glfwSetWindowSize(window, width, height);
    glfwSetWindowAspectRatio(window, width, height);

   // glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glViewport(0, 0, width, height);
    return true;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
}

int main()
{
    if (!setGLFW()) return -1;

    Instructions::Test();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        render();
    }

	return 1;
}