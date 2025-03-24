#include <signal.h>

#include "common.h"
#include "texture.h"
#include "shader_program.h"
#include "shader.h"
#include "ui_element.h"
#include "frametime.h"
#include "mesh.h"
#include "camera.h"
#include "materials.h"

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
            this->onClear();

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

    virtual int onClear() {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
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

struct MatrixShader : public MaterialShader {
    MatrixShader(const MaterialShader &base):MaterialShader(base){}
    MatrixShader(){}

    void use() override {
        MaterialShader::use();

        glEnable(GL_MULTISAMPLE);

        //glEnable(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);
        //glDepthFunc(GL_EQUAL);

        glEnable(GL_POLYGON_OFFSET_FILL);
        //glDepthMask(GL_TRUE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        
        glEnable(GL_CULL_FACE);
        //glDisable(GL_CULL_FACE);
        
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
        dummy_texture->generate({1.0f,1.0f,1.0f,0.5f});
        //object_shader = new shader_program_t(vertex_shader, fragment_shader);
        object_shader = new MatrixShader(MaterialShader(shader_program_t(vertex_shader, fragment_shader),
                        new material_t(dummy_texture, dummy_texture, 0.5f),
                        new light_t({0.0f,0,0}, {0.5,0.5,0.5}, {0.0,0.0,0.0}, {0.0,0.0,0.0})));

        led_object = new mesh_t;
        camera = new camera_t;

        return glsuccess;
    }

    int load() override {
        return

            vertex_shader->load("assets/shaders/vertex.glsl")
        ||  fragment_shader->load("assets/shaders/fragment.glsl")
        ||  object_shader->load()

        ||  led_object->loadObj("assets/models/LED_bulb.obj");
    }

    int onRender(double dt) override {
        object_shader->use();
        object_shader->set_camera(camera);
        //object_shader->set_v3("light.diffuse", glm::vec3(0));

        auto model_scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

        const int matrix_size = 8;

        auto &camera_position = camera->position;

        std::vector<glm::vec3> positions;
        int renderCount = 0;
        //positions.assign(matrix_size * matrix_size * matrix_size, glm::vec3(0.0f)); 
        for (int x = 0; x < matrix_size; x++) {
            for (int y = 0; y < matrix_size; y++) {
                for (int z = 0; z < matrix_size; z++) {
                    auto position = glm::vec3(x,y,z);
                    auto direction = glm::normalize(position - camera_position);
                    auto in_view = glm::dot(direction, camera->front);
                    
                    if (in_view < 0)
                        continue;
        
                    renderCount++;
        
                    positions.push_back({x,y,z});
                }
            }
        }

        std::sort(positions.begin(), positions.end(), [camera_position](const auto &a, const auto &b) {
            return glm::distance(a, camera_position) > glm::distance(b, camera_position);
        });

        for (auto &pos : positions) {
            auto model = glm::translate(glm::mat4(1.0f), pos);
            object_shader->set_m4("model", model * model_scale);
            led_object->render();
        }

        //fprintf(stderr, "%i models\n", renderCount);

        return glsuccess;
    }

    int onClear() override {
        glClearColor(0.5,0.5,0.5,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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