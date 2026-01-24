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
// Default path relative to the build folder
static char trace_path[256] = "../tests/test_fragmentation_stress.csv"; 

static std::vector<uint64_t> timestamps;
static int selected_timestamp_idx = -1;

// ---- CONTROL PANEL WINDOW ----
void draw_control_panel() {
    // FORCE Position: Top-Left (x=20, y=20)
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Always);

    ImGui::Begin("Simulation Control", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Configuration");
    ImGui::Separator();

    ImGui::InputText("Trace Path", trace_path, sizeof(trace_path));

    ImGui::Text("Strategy:");
    ImGui::RadioButton("First Fit", &fit_choice, 0); ImGui::SameLine();
    ImGui::RadioButton("Best Fit",  &fit_choice, 1); ImGui::SameLine();
    ImGui::RadioButton("Worst Fit", &fit_choice, 2);

    ImGui::Dummy(ImVec2(0.0f, 20.0f)); // Spacing

    if (ImGui::Button("RUN SIMULATION", ImVec2(380, 40))) {
        std::cout << "Starting simulation..." << std::endl;
        
        // Reset state
        timeline.clear();
        timestamps.clear();
        selected_timestamp_idx = -1;
        simulation_ran = false;

        SimConfig cfg;
        cfg.trace_file = trace_path;
        cfg.strategy = (fit_choice == 0) ? FitStrategy::FirstFit : 
                       (fit_choice == 1) ? FitStrategy::BestFit : FitStrategy::WorstFit;

        try {
            // Run the actual simulation logic
            bool success = run_simulation(cfg, timeline);
            
            if (success) {
                simulation_ran = true;
                // Populate timestamps for the slider/list
                for (auto const& [time, _] : timeline.all()) {
                    timestamps.push_back(time);
                }
                std::cout << "Simulation success. " << timestamps.size() << " steps recorded." << std::endl;
            } else {
                std::cerr << "Simulation failed (logic returned false)." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Simulation Exception: " << e.what() << std::endl;
        }
    }

    ImGui::End();
}

// ---- TIMELINE / RESULTS WINDOW ----
void draw_timeline() {
    // FORCE Position: Right Side (x=440, y=20)
    ImGui::SetNextWindowPos(ImVec2(440, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(800, 680), ImGuiCond_Always);

    ImGui::Begin("Timeline Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (!simulation_ran) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Waiting for simulation run...");
        ImGui::End();
        return;
    }

    if (timestamps.empty()) {
        ImGui::Text("Simulation ran but produced no events.");
        ImGui::End();
        return;
    }

    // 1. Timestamp Selector
    ImGui::Text("Select Time Step:");
    // Create a list box to select the timestamp
    if (ImGui::BeginListBox("##timestamps", ImVec2(150, 600))) {
        for (int i = 0; i < (int)timestamps.size(); i++) {
            char buf[32];
            sprintf(buf, "Time: %lu", timestamps[i]);
            const bool is_selected = (selected_timestamp_idx == i);
            
            if (ImGui::Selectable(buf, is_selected)) {
                selected_timestamp_idx = i;
            }

            // Set the initial focus when opening the box
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    ImGui::SameLine();

    // 2. Event Details Area
    ImGui::BeginChild("Details", ImVec2(0, 600), true);
    
    if (selected_timestamp_idx >= 0 && selected_timestamp_idx < (int)timestamps.size()) {
        uint64_t t = timestamps[selected_timestamp_idx];
        auto snap_opt = timeline.get(t); // Get snapshot from timeline

        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Events at Time: %lu", t);
        ImGui::Separator();

        if (snap_opt) {
            for (auto &e : snap_opt->events) {
                // Color code based on Process ID for readability
                ImVec4 color = (e.pid == 0) ? ImVec4(0.8f, 0.8f, 0.8f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                ImGui::TextColored(color, "[PID %u] %s", e.pid, e.message.c_str());
            }
        } else {
            ImGui::Text("No data snapshot found.");
        }
    } else {
        ImGui::Text("Select a timestamp from the left list to view details.");
    }

    ImGui::EndChild();

    ImGui::End();
}

// ---- MAIN ----
int main() {
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    });

    if (!glfwInit()) return 1;

    // Use default OpenGL (Legacy/Compatibility) which works on your Arch setup
    // Do NOT set Core Profile hints here.
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Simulator GUI", nullptr, nullptr);
    if (window == nullptr) return 1;
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Init Backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130"); // Use legacy shader version

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw our Panels
        draw_control_panel();
        draw_timeline();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        // Dark Grey Background
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
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
