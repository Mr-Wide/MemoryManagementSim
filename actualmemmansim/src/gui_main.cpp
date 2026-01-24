#include "sim/simrunner.h"
#include "sim/timeline.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h> // Will drag in OpenGL headers

#include <iostream>
#include <vector>
#include <cstdio>

// Use standard namespace for simplicity in this file
using namespace sim;

static Timeline timeline;
static bool simulation_ran = false;
static int fit_choice = 0;
static char trace_path[256] = "../tests/test_fragmentation_stress.csv"; 
static std::vector<uint64_t> timestamps;
static int selected_timestamp = -1;

// ---- CONTROL PANEL ----
void draw_control_panel() {
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    
    // Begin the window. If this returns false, the window is collapsed/closed.
    if (!ImGui::Begin("Simulation Control")) {
        ImGui::End();
        return;
    }

    ImGui::Text("System Status: %s", "Running"); // Debug text
    ImGui::Separator();

    ImGui::InputText("Trace file", trace_path, sizeof(trace_path));
    ImGui::RadioButton("First Fit", &fit_choice, 0);
    ImGui::RadioButton("Best Fit",  &fit_choice, 1);
    ImGui::RadioButton("Worst Fit", &fit_choice, 2);

    if (ImGui::Button("Run Simulation")) {
        std::cout << "Button Clicked!" << std::endl;
        // ... (simulation logic skipped for debug simplicity) ...
    }

    ImGui::End();
}

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// ---- MAIN ----
int main() {
    glfwSetErrorCallback(glfw_error_callback);
    
    if (!glfwInit()) return 1;

    // --------------------------------------------------------
    // LINUX COMPATIBILITY SETUP
    // --------------------------------------------------------
    // We request OpenGL 3.3 Core. This is the sweet spot for Arch/Linux.
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 

    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Simulator", nullptr, nullptr);
    if (window == nullptr) return 1;
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // FORCE DEFAULT FONT (Helps if font texture is missing)
    io.Fonts->AddFontDefault();

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool show_demo_window = true;
    int frame_count = 0;

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Draw Official Demo Window (To test if ImGui is working at all)
        ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Draw Your Custom Panel
        draw_control_panel();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        // Distinct color (Dark Teal)
        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // DEBUG: Check if ImGui actually has data to draw
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (frame_count % 60 == 0) { // Print once per second
            if (draw_data && draw_data->TotalVtxCount > 0) {
               // If you see this, ImGui IS generating buttons. 
               // If you still see nothing, it's a Shader/Monitor issue.
               std::cout << "Frame " << frame_count << ": ImGui drawing " << draw_data->TotalVtxCount << " vertices." << std::endl;
            } else {
               std::cout << "Frame " << frame_count << ": ImGui has NOTHING to draw." << std::endl;
            }
        }

        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
        glfwSwapBuffers(window);
        frame_count++;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
