#include "sim/simrunner.h"
#include "sim/timeline.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

using namespace sim;

static Timeline timeline;
static bool simulation_ran = false;
static int fit_choice = 0;
static char trace_path[256] = "../tests/test_fragmentation_stress.csv"; 
static std::vector<uint64_t> timestamps;
static int selected_timestamp = -1;

void draw_control_panel() {
    ImGui::Begin("Simulation Control");
    ImGui::Text("If you can see this, it works!"); // Simple test text
    
    ImGui::InputText("Trace file", trace_path, sizeof(trace_path));
    ImGui::RadioButton("First Fit", &fit_choice, 0);
    ImGui::RadioButton("Best Fit",  &fit_choice, 1);
    ImGui::RadioButton("Worst Fit", &fit_choice, 2);

    if (ImGui::Button("Run Simulation")) {
        // Simple logic just to test the button
        timeline.clear();
        timestamps.clear();
        SimConfig cfg;
        cfg.trace_file = trace_path;
        cfg.strategy = (fit_choice == 0) ? FitStrategy::FirstFit : 
                       (fit_choice == 1) ? FitStrategy::BestFit : FitStrategy::WorstFit;
        
        try {
            simulation_ran = run_simulation(cfg, timeline);
            if(simulation_ran) {
                for (auto &[time, _] : timeline.all()) timestamps.push_back(time);
            }
        } catch (...) {}
    }
    ImGui::End();
}

void draw_timeline() {
    ImGui::Begin("Timeline");
    if (timestamps.empty()) {
        ImGui::Text("No results yet.");
    } else {
        ImGui::Text("Results found: %lu timestamps", timestamps.size());
    }
    ImGui::End();
}

int main() {
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    });

    if (!glfwInit()) return 1;

    // -----------------------------------------------------------
    // CHANGE 1: REMOVE CORE PROFILE HINTS (Use Default/Legacy)
    // -----------------------------------------------------------
    // We do NOT set GLFW_CONTEXT_VERSION_MAJOR/MINOR here.
    // We let the driver pick the default (usually 2.1 or compatibility mode).
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Simulator (Safe Mode)", nullptr, nullptr);
    if (window == nullptr) return 1;
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    
    // -----------------------------------------------------------
    // CHANGE 2: USE OLDER SHADER VERSION
    // -----------------------------------------------------------
    ImGui_ImplOpenGL3_Init("#version 130"); 

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_control_panel();
        draw_timeline();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        // -----------------------------------------------------------
        // CHANGE 3: BRIGHT RED BACKGROUND
        // -----------------------------------------------------------
        // If you see RED, OpenGL is working.
        // If you see BLACK, OpenGL is broken.
        glClearColor(0.8f, 0.1f, 0.1f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
