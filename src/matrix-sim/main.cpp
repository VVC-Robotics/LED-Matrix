#include <signal.h>

#include "common.h"
#include "texture.h"
#include "shader_program.h"
#include "shader.h"
#include "ui_element.h"
#include "frametime.h"
#include "mesh.h"
#include "camera.h"

struct ProgramBase {
    ProgramBase(std::string title = "", int width = 800, int height = 800)
        :title(title),
         width(width),
         height(height) { }

    std::vector<texture_t*> textures;
    std::vector<shader_t*> shaders;
    std::vector<shader_program_t*> programs;
    std::vector<mesh_t*> meshes;
    ui_element_t ui_base;
    gui::frametime_t frametime;
    GLFWwindow *window;
    int width, height;
    std::string title;

    virtual int init_context() {
        if (!glfwInit())
            handle_error("Failed to init glfw");
        
        window = glfwCreateWindow(width, height, title.c_str(), 0, 0);

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

        glfwGetWindowSize(window, &width, &height);
        framebuffersize_callback(window, width, height);

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
        current_window[2] = width; //change how this works
        current_window[3] = height;

        glViewport(0,0,width,height);

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

struct material_shader_t : public shader_program_t {
    material_shader_t(const shader_program_t &base, material_t *material, light_t *light):shader_program_t(base),material(material),light(light){}
    material_shader_t(){}

    material_t *material;
    light_t *light;

    void use() override {
        shader_program_t::use();

        material->use();

        set_v3("light.position", light->position);
        set_v3("light.ambient", light->ambient);
        set_v3("light.specular", light->specular);
        set_v3("light.diffuse", light->diffuse);
        set_f("material.shininess", material->shininess);
    }
};

struct matrix_shader_t : public material_shader_t {
    matrix_shader_t(const material_shader_t &base):material_shader_t(base){}
    matrix_shader_t(){}

    void use() override {
        material_shader_t::use();

        glEnable(GL_MULTISAMPLE);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    }
};

struct MatrixSim : public ProgramBase {
    MatrixSim(const ProgramBase &base):ProgramBase(base){}
    MatrixSim(){}

    camera_t *camera;

    shader_t *vertex_shader;
    shader_t *fragment_shader;
    shader_program_t *object_shader;
    texture_t *dummy_texture;

    mesh_t *led_object;

    int init_context() override {
        if (!glfwInit())
            return glfail;

        glfwWindowHint(GLFW_SAMPLES, 4);

        return ProgramBase::init_context();
    }

    int init() override {
        vertex_shader = new shader_t(GL_VERTEX_SHADER);
        fragment_shader = new shader_t(GL_FRAGMENT_SHADER);
        dummy_texture = new texture_t();
        dummy_texture->generate({0,0.0f,0.5f,0.5f});
        //object_shader = new shader_program_t(vertex_shader, fragment_shader);
        object_shader = new matrix_shader_t(material_shader_t(shader_program_t(vertex_shader, fragment_shader),
                        new material_t(dummy_texture, dummy_texture, 0.5f),
                        new light_t({20.0f,0,0}, {0.2,0.2,0.2}, {0.2,0.2,0.2}, {0.2,0.2,0.2})));

        led_object = new mesh_t;
        camera = new camera_t;

        return glsuccess;
    }

    int load() override {
        return

            vertex_shader->load("assets/shaders/vertex.glsl")
        ||  fragment_shader->load("assets/shaders/fragment.glsl")
        ||  object_shader->load()

        ||  led_object->loadObj("assets/models/LED.obj");
    }

    int onRender(double dt) override {
        object_shader->use();
        object_shader->set_camera(camera);

        const int matrix_size = 16;
        for (int x = 0; x < matrix_size; x++) {
            for (int y = 0; y < matrix_size; y++) {
                for (int z = 0; z < matrix_size; z++) {
                    glm::mat4 matrix(1.0f);
                    matrix = glm::translate(matrix, glm::vec3(x, y, z));
                    matrix = glm::scale(matrix, glm::vec3(0.05f));
                    object_shader->set_m4("model", matrix);
                    led_object->render();
                }
            }
        }

        return glsuccess;
    }

    int onFrame(double dt) override {
        camera->keyboard(window, dt);

        return glsuccess;
    }

    int onMouse(int button, int action, int mods) override {
        camera->mousePress(window, button, action, mods);

        return glsuccess;
    }

    int onCursor(double x, double y) override {
        camera->mouseMove(window, x, y);

        return glsuccess;
    }
};

int main() {
    MatrixSim sim = ProgramBase("LED Matrix Simulation");

    if (sim.init_context() || sim.init() || sim.load())
        sim.handle_error("Failed to start program");

    return sim.run();
}