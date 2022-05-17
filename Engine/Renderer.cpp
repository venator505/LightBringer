#include "Renderer.h"
#include "Camera.h"
namespace ylb {
void Renderer::ProcessInput(double &&delta_time) {
    double move_speed = 10.0 * delta_time;
    double rot_speed = ylb::PI / 3 * delta_time;
    int dx = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ? -1 : glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ? 1 :
                                                                                                                0;
    int dz = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ? 1 : glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ? -1 :
                                                                                                               0;
    int dy = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ? 1 : 0;
    dy = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS ? -1 : dy;

    int ty = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ? 1 : 0;
    ty = glfwGetKey(window, GLFW_KEY_RIGHT) ? -1 : ty;

    if (dx || dz || dy) {
        glm::vec3 vx = glm::vec3{1, 0, 0} * static_cast<float>(dx * move_speed);
        glm::vec3 vz = glm::vec3{0, 0, 1} * static_cast<float>(dz * move_speed);
        glm::vec3 vy = glm::vec3{0, 1, 0} * static_cast<float>(dy * move_speed);
        glm::vec3 newPos = cam->transform.WorldPosition() + vx + vz + vy;
        cam->transform.SetPosition(newPos);
        transformer->set_world_to_view(cam);
    }
}

void Renderer::Framebuffer_Size_Callback(GLFWwindow *window, int width, int height) {
    auto &instance = Instance();
    delete instance.frame_buffer;
    instance.w = width;
    instance.h = height;
    instance.frame_buffer = new FrameBuffer(width, height);
    instance.cam->aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
    instance.cam->UpdateProjectionInfo();
    instance.SetMVPMatrix(instance.cam, PROJECTION_MODE::PERSPECTIVE);
}

void Renderer::Render(std::vector<ylb::Mesh> &meshs) {
    statistic.InitTriangleCnt();
    for (int i = 0 ; i < world.size() ; i++) {
        auto& mesh = world[i];
        auto triangles = mesh.Triangles();

        for (auto& t : *triangles) {
            VertexShaderContext vertexShaderContext;
            vertexShaderContext.model = &mesh.transform.ModelMatrix();
            vertexShaderContext.view = &transformer->view;
            vertexShaderContext.project = &transformer->projection;
            vertexShaderContext.camPos = &cam->transform.WorldPosition();
            ProcessGeometry(t, mesh.shader, vertexShaderContext);
        }
    }
}

void Renderer::Rasterization(ylb::Triangle &t, Shader *shader, Light *light) {
    statistic.IncreaseTriangleCnt();
    const ylb::BoundingBox *bb = t.bounding_box();
    glm::vec3 color = {0.5, 0.5, 0.5};
    FragmentShaderContext fragmentShaderContext(&cam->transform.WorldPosition(),lights[0]);
    int min_left = static_cast<int>(ylb::max(bb->bot, 0.f));
    for (int y = min_left; y < bb->top; y++)
        for (int x = bb->left; x < bb->right; x++) {
            int pixel = y * w + x;
            //三角形测试
            if (ylb::Triangle::inside(x + 0.5f, y + 0.5f, t)) {
                float depth = 0;

                if (cam->mode == PROJECTION_MODE::PERSPECTIVE)
                    depth = t.interpolated_depth();
                else
                    depth = t.vts[0].sz() * t.cof.x + t.vts[1].sz() * t.cof.y + t.vts[2].sz() * t.cof.z;
                //深度测试
                if (depth - frame_buffer->depth[pixel] >= ylb::eps) {
                    //深度写入
                    if (renderTargetSetting->open_depth_buffer_write) {
                        frame_buffer->set_depth(x, y, depth);
                    }
                    //帧缓存写入
                    if (renderTargetSetting->open_frame_buffer_write) {
                        color = shader->FragmentShading(t,fragmentShaderContext);
                        frame_buffer->set_color(x, y, color);
                    }
                }
            }
        }
}

void Renderer::InitOpenGL() {
    //设置opengl
    if (!glfwInit()) {
        printf("Couldn't init GLFW\n");
        return;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
    // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

    window = glfwCreateWindow(w, h, title, NULL, NULL);
    if (!window) {
        printf("Couldn't open window\n");
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, &Renderer::Framebuffer_Size_Callback);

    glfwSwapInterval(1); // Enable vsync
                         // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
    // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
    // Enable Gamepad Controls
}

void Renderer::LoadScene(const char* scene_file_path)
{
    auto scene = SceneLoader::Instance().LoadScene(scene_file_path);
    cam = std::move(scene->cam);
    cam->aspect_ratio = static_cast<float>(w) / static_cast<float>(h);
    cam->UpdateProjectionInfo();
    SetMVPMatrix(cam, PROJECTION_MODE::PERSPECTIVE);
    for (auto& obj : *scene->meshs) {
        world.push_back(*obj);
    }

    for (auto& lite : *scene->lights) {
        lights.push_back(lite);
    }
}

void Renderer::SetMVPMatrix(Camera *cam, PROJECTION_MODE mode) {
    transformer->set_world_to_view(cam);
    transformer->set_view_to_project(cam, mode);
    transformer->set_projection_to_screen(w, h);
}

void Renderer::Start() {
    using ylb::Triangle;
    using ylb::Vertex;

    InitOpenGL();

    LoadScene("Scene/sample.json");
    

    while (!glfwWindowShouldClose(window)) {
        ProcessInput(ImGui::GetIO().Framerate / 1000);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        frame_buffer->clear();

        // renderTargetSetting->open_z_buffer_write = false;
        // render(world_only_sky);
        renderTargetSetting->open_depth_buffer_write = true;
        Render(world);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        statistic.Render();
        glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, frame_buffer->pixels);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

Renderer::Renderer(int _w, int _h) :
    w(_w), h(_h) {
    frame_buffer = new FrameBuffer(w, h);
    transformer = new Transformer();
    renderTargetSetting = new RenderTargetSetting();
}

bool Renderer::Backface_culling(Vertex &vt) {
    return false;
}

void Renderer::ProcessGeometry(Triangle &t, Shader *&shader, const VertexShaderContext& vertexShaderContext) {
    for (int i = 0; i < 3; i++) {
        auto &vt = t.vts[i];
        vt.ccv = shader->VertexShading(vt, vertexShaderContext);
    }

    //裁剪

    std::vector<Vertex> vts{t.vts[0], t.vts[1], t.vts[2]};

    auto clipped_x = Clipper::ClipPolygon(Plane::POSITIVE_X, vts);
    auto clipped_nx = Clipper::ClipPolygon(Plane::NEGATIVE_X, clipped_x);
    auto clipped_y = Clipper::ClipPolygon(Plane::POSITIVE_Y, clipped_nx);
    auto clipped_ny = Clipper::ClipPolygon(Plane::NEGATIVE_Y, clipped_y);
    auto clipped_z = Clipper::ClipPolygon(Plane::POSITIVE_Z, clipped_ny);
    auto clipped_nz = Clipper::ClipPolygon(Plane::NEGATIVE_Z, clipped_z);
    auto clipped_vt = Clipper::ClipPolygon(Plane::POSITIVE_W, clipped_nz);

    if (clipped_vt.size() < 3)
        return;

    for (int i = 0; i < clipped_vt.size(); i++) {
        auto &vt = clipped_vt[i];
        auto &ccv_pos = vt.ccv;
        //透视除法
        vt.inv = 1.0f / ccv_pos.w;
        ccv_pos *= vt.inv;

        if (cam->mode == PROJECTION_MODE::PERSPECTIVE) {
            vt.tex_coord *= vt.inv;
            vt.normal = vt.normal * vt.inv;
            vt.position = vt.world_position * vt.inv;
        }

        vt.sv_pos = ccv_pos * transformer->view_port;
    }

    int step = clipped_vt.size() > 3 ? 1 : 3;
    int triangle_num = clipped_vt.size();
    for (int i = 0; i < triangle_num; i += step) {
        Triangle t(clipped_vt[i], clipped_vt[(i + 1) % clipped_vt.size()],
                   clipped_vt[(i + 2) % clipped_vt.size()]);
        //设置三角形,准备光栅化
        t.ready_rasterization();
        Rasterization(t, shader, nullptr);
    }
}

} // namespace ylb