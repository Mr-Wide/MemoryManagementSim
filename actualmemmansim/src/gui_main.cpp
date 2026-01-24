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

// Default path (Updated to match your likely test location based on your command)
static char trace_path[256] = "../tests/test_fragmentation_stress.csv";

static std::vector<uint64_t> timestamps;
static int selected_timestamp = -1;

// ---- CONTROL PANEL ----
void draw_control_panel() {
    ImGui::Begin("Simulation Control");

    ImGui::InputText("Trace file", trace_path, sizeof(trace_path));

    ImGui::RadioButton("First Fit", &fit_choice, 0);
    ImGui::RadioButton("Best Fit",  &fit_choice, 1);
    ImGui::RadioButton("Worst Fit", &fit_choice, 2);

    if (ImGui::Button("Run Simulation")) {
        std::cout << "Button Pressed! Running simulation on: " << trace_path << std::endl;
        
        timeline.clear();
        timestamps.clear();
        selected_timestamp = -1;

        SimConfig cfg;
        cfg.trace_file = trace_path;
        cfg.strategy =
            fit_choice == 0 ? FitStrategy::FirstFit :
            fit_choice == 1 ? FitStrategy::BestFit  :
                              FitStrategy::WorstFit;

        // Try-catch block to prevent GUI crash if logic fails
        try {
            simulation_ran = run_simulation(cfg, timeline);
        } catch (const std::exception& e) {
            std::cerr << "Simulation Error: " << e.what() << std::endl;
            simulation_ran = false;
        }

        // Fill timestamps
        if (simulation_ran) {
            for (auto &[time, _] : timeline.all())
                timestamps.push_back(time);
        }
    }

    ImGui::End();
}

// ---- TIMELINE PANEL ----
void draw_timeline() {
    ImGui::Begin("Timeline");

    if (!simulation_ran) {
        ImGui::Text("Ready. Check settings and press 'Run Simulation'.");
        ImGui::End();
        return;
    }

    // Left column: Timestamps
    ImGui::BeginChild("Timestamps", ImVec2(150, 0), true);
    for (int i = 0; i < timestamps.size(); i++) {
        char buf[32];
        sprintf(buf, "%lu", timestamps[i]);
        if (ImGui::Selectable(buf, selected_timestamp == i)) {
            selected_timestamp = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right column: Events
    ImGui::BeginChild("Events", ImVec2(0, 0), true);
    if (selected_timestamp >= 0 && selected_timestamp < timestamps.size()) {
        uint64_t t = timestamps[selected_timestamp];
        auto snap_opt = timeline.get(t);
        if (snap_opt) {
            ImGui::TextColored(ImVec4(1,1,0,1), "Time: %lu", t);
            ImGui::Separator();
            for (auto &e : snap_opt->events)
                ImGui::BulletText("PID %u: %s", e.pid, e.message.c_str());
        } else {
            ImGui::Text("No events data found for this timestamp.");
        }
    } else {
        ImGui::Text("Select a timestamp to inspect memory state.");
    }
    ImGui::EndChild();

    ImGui::End();
}

// ---- MAIN ----
int main() {
    // 1. Setup Error Callback
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    });

    if (!glfwInit()) return 1;

    // 2. Setup OpenGL 3.3 Core Profile (CRITICAL FOR LINUX)
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac

    // 3. Create Window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Simulator", nullptr, nullptr);
    if (window == nullptr) return 1;
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // 4. Setup ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // 5. Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 6. Main Loop
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
        
        // Clear with a color so you know the window is alive
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 7. Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
