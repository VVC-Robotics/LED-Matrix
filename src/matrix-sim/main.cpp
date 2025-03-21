#include <signal.h>

#include "common.h"
#include "texture.h"
#include "shader_program.h"
#include "shader.h"
#include "ui_element.h"
#include "frametime.h"

struct ProgramBase {
    std::vector<texture_t*> textures;
    std::vector<shader_t*> shaders;
    std::vector<shader_program_t*> programs;
    ui_element_t ui_base;
    gui::frametime_t frametime;
    GLFWwindow *window;

    virtual int init_context() {
        if (!glfwInit())
            handle_error("Failed to init glfw");
        
        window = glfwCreateWindow(800, 800, "ProgramBase", 0, 0);

        if (!window)
            handle_error("Failed to create glfw window");

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        static ProgramBase *current;
        current = this;
        
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
            current->framebuffersize_callback(window, width, height);
        });
        glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
            current->cursorpos_callback(window, x, y);
        });
        glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods){
            current->mousebutton_callback(window, button, action, mods);
        });

        signal(SIGINT, [](int sig) {
            current->on_signal(sig);
        });

        return glsuccess;
    }

    virtual int init() {
        return glsuccess;
    }

    virtual int load() {
        return glsuccess;
    }

    virtual int run() {
        while (!glfwWindowShouldClose(window)) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            frametime.update();
            auto delta_time = frametime.get_delta_time<double>();

            this->onFrame(delta_time);
            this->onKeyboard(delta_time);

            this->onRender(delta_time);
            ui_base.render();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        return glsuccess;
    }

    virtual int onRender(double delta_time) {
        return glsuccess;
    }

    virtual int onFrame(double delta_time) {
        return glsuccess;
    }

    virtual int onFramebuffer(int width, int height) {
        ui_base.onFramebuffer(width, height);

        return glsuccess;
    }

    virtual int onCursor(double x, double y) {
        ui_base.onCursor(x, y);

        return glsuccess;
    }

    virtual int onMouse(int button, int action, int mods) {
        ui_base.onMouse(button, action, mods);

        return glsuccess;
    }

    virtual int onKeyboard(double delta_time) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            hint_exit();

        ui_base.onKeyboard(delta_time);

        return glsuccess;
    }

    virtual void on_signal(int signal) {
        fprintf(stderr, "Signal caught %i\n", signal);
    }

    virtual void destroy() {

    }

    virtual void reset() {

    }

    virtual void safe_exit(int errcode) {
        destroy();
        exit(errcode);
    }

    virtual void handle_error(const char *errstr, int errcode=-1) {
        fprintf(stderr, "Error: %s\n", errstr);
        safe_exit(errcode);
    }

    virtual void handle_errno(const char *errstr) {
        int errsv = errno;
        fprintf(stderr, "Error: %s, Errno: %i %s\n", errstr, errsv, strerror(errsv));
        hint_exit();
    }

    virtual void hint_exit() {
        glfwSetWindowShouldClose(window, 1);
    }

    void framebuffersize_callback(GLFWwindow *window, int width, int height) {
        onFramebuffer(width, height);
    }

    void cursorpos_callback(GLFWwindow *window, double x, double y) {
        onCursor(x, y);
    }

    void mousebutton_callback(GLFWwindow *window, int button, int action, int mods) {
        onMouse(button, action, mods);
    }

    void keyboard_callback(GLFWwindow *window, double delta_time) {
        onKeyboard(delta_time);
    }
};

struct MatrixSim : public ProgramBase {

};

int main() {
    MatrixSim sim;

    if (sim.init_context() || sim.init() || sim.load())
        sim.handle_error("Failed to start program");

    return sim.run();
}