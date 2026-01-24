#include "sim/simrunner.h"
#include "sim/timeline.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>

using namespace sim;

// ---- GLOBAL STATE ----
static Timeline timeline;
static bool simulation_ran = false;
static int fit_choice = 0;
static char trace_path[256] = "../tests/test_fragmentation_stress.csv"; 

static std::vector<uint64_t> timestamps;
static int selected_timestamp_idx = -1;

// ---- CONTROL PANEL ----
void draw_control_panel() {
    // FORCE POSITION: Top-Left
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);

    ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Trace File:");
    ImGui::InputText("##trace", trace_path, sizeof(trace_path));

    ImGui::Text("Strategy:");
    ImGui::RadioButton("First Fit", &fit_choice, 0); ImGui::SameLine();
    ImGui::RadioButton("Best Fit",  &fit_choice, 1); ImGui::SameLine();
    ImGui::RadioButton("Worst Fit", &fit_choice, 2);

    ImGui::Dummy(ImVec2(0, 20));

    if (ImGui::Button("RUN SIMULATION", ImVec2(380, 50))) {
        std::cout << "[GUI] Run button pressed..." << std::endl;
        
        timeline.clear();
        timestamps.clear();
        selected_timestamp_idx = -1;

        SimConfig cfg;
        cfg.trace_file = trace_path;
        cfg.strategy = (fit_choice == 0) ? FitStrategy::FirstFit : 
                       (fit_choice == 1) ? FitStrategy::BestFit : FitStrategy::WorstFit;

        try {
            if (run_simulation(cfg, timeline)) {
                simulation_ran = true;
                for (auto const& [time, _] : timeline.all()) {
                    timestamps.push_back(time);
                }
                std::cout << "[GUI] Simulation finished. Steps: " << timestamps.size() << std::endl;
            } else {
                std::cout << "[GUI] Simulation returned false." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[GUI] Error: " << e.what() << std::endl;
        }
    }
    ImGui::End();
}

// ---- TIMELINE VIEWER ----
void draw_timeline() {
    // FORCE POSITION: Right Side
    ImGui::SetNextWindowPos(ImVec2(440, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(800, 680), ImGuiCond_Always);

    ImGui::Begin("Timeline Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (!simulation_ran) {
        ImGui::Text("Simulation not run yet.");
        ImGui::End();
        return;
    }

    // Left ListBox for Timestamps
    ImGui::BeginChild("TimeList", ImVec2(150, 0), true);
    for (int i = 0; i < (int)timestamps.size(); i++) {
        char buf[32];
        // FIX: Cast to (unsigned long long) to suppress warnings
        sprintf(buf, "%llu", (unsigned long long)timestamps[i]);
        if (ImGui::Selectable(buf, selected_timestamp_idx == i)) {
            selected_timestamp_idx = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right Detail View
    ImGui::BeginChild("Details", ImVec2(0, 0), true);
    if (selected_timestamp_idx >= 0 && selected_timestamp_idx < (int)timestamps.size()) {
        uint64_t t = timestamps[selected_timestamp_idx];
        auto snap_opt = timeline.get(t);
        
        if (snap_opt) {
            ImGui::TextColored(ImVec4(0,1,1,1), "Time Step: %llu", (unsigned long long)t);
            ImGui::Separator();
            
            // Memory Metrics
            ImGui::Text("Allocated: %llu bytes", (unsigned long long)snap_opt->metrics.allocated_bytes);
            ImGui::Text("Free:      %llu bytes", (unsigned long long)snap_opt->metrics.free_bytes);
            ImGui::Separator();

            // Events
            for (auto &e : snap_opt->events) {
                ImGui::BulletText("[PID %u] %s", e.pid, e.message.c_str());
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

// ---- MAIN ----
int main() {
    // DEBUG: Start
    std::cout << "[DEBUG] 1. Starting main..." << std::endl;

    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "[GLFW ERROR] " << error << ": " << description << std::endl;
    });

    // DEBUG: Init GLFW
    std::cout << "[DEBUG] 2. Initializing GLFW..." << std::endl;
    if (!glfwInit()) {
        std::cerr << "[FATAL] Failed to initialize GLFW!" << std::endl;
        return 1;
    }

    // DEBUG: Create Window
    std::cout << "[DEBUG] 3. Creating Window (Legacy Profile)..." << std::endl;
    
    // We intentionally do NOT use Core Profile hints to ensure maximum compatibility
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Simulator GUI", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "[FATAL] Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    std::cout << "[DEBUG] 4. Context Current..." << std::endl;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    // DEBUG: Init ImGui
    std::cout << "[DEBUG] 5. Initializing ImGui..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // DEBUG: Init Backends
    std::cout << "[DEBUG] 6. Initializing ImGui Backends..." << std::endl;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::cout << "[DEBUG] 7. Entering Main Loop..." << std::endl;

    // Main Loop
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
        
        // RED BACKGROUND to verify OpenGL is drawing
        glClearColor(0.5f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    std::cout << "[DEBUG] 8. Cleanup..." << std::endl;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
