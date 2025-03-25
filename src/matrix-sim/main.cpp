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
#include "ui_shader.h"
#include "ui_slider.h"
#include "util.h"

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
        ProgramBase::onFramebuffer(width, height);

        return glsuccess;
    }

    virtual int init() {
        return onFramebuffer(width, height);
    }

    virtual int load() {
        return ui_base.load();
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
        return ui_base.render();
    }

    virtual int onFrame(double delta_time) {
        return glsuccess;
    }

    virtual int onFramebuffer(int width, int height) {
        current_window[2] = width; //change how this works
        current_window[3] = height;
        this->width = width;
        this->height = height;

        glViewport(0,0,width,height);

        return ui_base.onFramebuffer(width, height);
    }

    virtual int onCursor(double x, double y) {
        return ui_base.onCursor(x, y);
    }

    virtual int onMouse(int button, int action, int mods) {
        return ui_base.onMouse(button, action, mods);
    }

    virtual int onKeyboard(double delta_time) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            hint_exit();

        return ui_base.onKeyboard(delta_time);
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
    texture_t *text_texture;
    gui::UIShader *ui_shader;
    ui_text_t *ui_debug;

    mesh_t *led_object;

    void get_debug_info() {
        const auto vtos = [](const glm::vec4 &vec) {
            return std::format("{:>6.2f} {:>6.2f} {:>6.2f} {:>6.2f}\n", vec.x, vec.y, vec.z, vec.w);
        };

        const auto vtos2 = [](const glm::vec2 &vec) {
            return std::format("{:>6.2f} {:>6.2f}\n", vec.x, vec.y);
        };

        const auto mtos = [vtos](const glm::mat4 &matrix) {
            return vtos(matrix[0]) + vtos(matrix[1]) + vtos(matrix[2]) + vtos(matrix[3]);
        };

        auto mat = ui_shader->projection;
        auto screen = glm::vec2(width, height);
        auto scale = glm::normalize(screen);
        auto eq = 1.0f/sqrtf(2.0f);
        auto inv = eq/scale;
        
        std::string info;

        info += std::format("FPS: {:>6.2f} ft: {:>6.2f} ms\n", frametime.get_fps(), frametime.get_ms());
        info += std::format("value: {}\nHELLO\n", ui_shader->mixFactor);
        info += "Projection:\n" + mtos(mat);
        info += std::format("Screen: {}", vtos2(screen));
        info += std::format("Scale : {}", vtos2(scale));
        info += std::format("Invert: {}", vtos2(inv));
        info += std::format("Ratio : {:>6.2f}\n", mat[1][1] / mat[3][1]);

        ui_debug->set_string(info);
    }

    int init_context() override {
        if (!glfwInit())
            return glfail;

        glfwWindowHint(GLFW_SAMPLES, 4);

        return ProgramBase::init_context();
    }

    int init() override {
        vertex_shader = new shader_t(GL_VERTEX_SHADER);
        fragment_shader = new shader_t(GL_FRAGMENT_SHADER);
        text_texture = new texture_t;
        dummy_texture = new texture_t();
        dummy_texture->generate({1.0f,1.0f,1.0f,0.5f});
        //object_shader = new shader_program_t(vertex_shader, fragment_shader);
        object_shader = new MatrixShader(MaterialShader(shader_program_t(vertex_shader, fragment_shader),
                        new material_t(dummy_texture, dummy_texture, 0.5f),
                        new light_t({0.0f,0,0}, {0.5,0.5,0.5}, {0.0,0.0,0.0}, {0.0,0.0,0.0})));
        ui_shader = new gui::UIShader(new gui::VertexShader("assets/shaders/text_vertex.glsl"), new gui::FragmentShader("assets/shaders/text_fragment.glsl"));
        ui_base.add_child(new gui::UISliderSocketable(
                        ui_slider_t(window, ui_shader, text_texture, glm::vec4{0.45, -0.95, 0.5, 0.1}, -1.0f, 1.0f, 0.0f, "mixFactor", true),
                        gui::UISliderSocketPtr<>(&(ui_shader->mixFactor))));
        ui_debug = ui_base.add_child(new ui_text_t(window, ui_shader, text_texture, {-1.0f,-1.0f,1.0f,1.0f}, "", std::bind(&MatrixSim::get_debug_info, this)));

        led_object = new mesh_t;
        camera = new camera_t;

        ProgramBase::init();

        return glsuccess;
    }

    int load() override {
        return

            vertex_shader->load("assets/shaders/vertex.glsl")
        ||  fragment_shader->load("assets/shaders/fragment.glsl")
        ||  object_shader->load()
        ||  ui_shader->load()
        ||  text_texture->load("assets/textures/text.png")
        ||  ui_base.load()
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

        ui_base.render();
        //fprintf(stderr, "%i models\n", renderCount);

        return glsuccess;
    }

    int onClear() override {
        glClearColor(0.5,0.5,0.5,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return glsuccess;
    }

    int onKeyboard(double dt) override {
        if (ProgramBase::onKeyboard(dt))
            return glsuccess;

        camera->keyboard(window, dt);

        return glsuccess;
    }

    int onMouse(int button, int action, int mods) override {
        if (ProgramBase::onMouse(button, action, mods))
            return glsuccess;

        camera->mousePress(window, button, action, mods);

        return glsuccess;
    }

    int onCursor(double x, double y) override {
        auto transform = util::project_screen(x, y, ui_shader->get_projection());

        if (ProgramBase::onCursor(transform.x, transform.y))
            return glsuccess;

        camera->mouseMove(window, x, y);

        return glsuccess;
    }

    int onFramebuffer(int width, int height) override {
        auto &proj = ui_shader->projection; 
        auto aspect = float(width) / height;

        ui_shader->set_buffer_size(width, height);

        return ProgramBase::onFramebuffer(width, height);
    }
};

int main() {
    MatrixSim sim = ProgramBase("LED Matrix Simulation");

    if (sim.init_context() || sim.init() || sim.load())
        sim.handle_error("Failed to start program");

    return sim.run();
}