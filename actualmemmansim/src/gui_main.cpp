#include <cstdio>
#include <iostream>
#include <vector>

// 1. Include only standard headers first to test the binary
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

// 2. Include your sim headers
#include "sim/simrunner.h"
#include "sim/timeline.h"

// USE NO GLOBALS!
// Everything must be inside main() or passed as arguments.

int main() {
    // 1. Force disable output buffering (So you see text INSTANTLY)
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("--- STEP 1: Program Started ---\n");

    // 2. Initialize Variables LOCALLY
    sim::Timeline timeline;
    std::vector<uint64_t> timestamps;
    int selected_timestamp_idx = -1;
    bool simulation_ran = false;
    int fit_choice = 0;
    char trace_path[256] = "../tests/test_fragmentation_stress.csv";

    printf("--- STEP 2: Variables Initialized ---\n");

    // 3. Error Callback
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    });

    // 4. Init GLFW
    if (!glfwInit()) {
        fprintf(stderr, "FAILED to init GLFW\n");
        return 1;
    }
    printf("--- STEP 3: GLFW Initialized ---\n");

    // 5. Create Window (Legacy Mode for safety)
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Simulator", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "FAILED to create Window\n");
        return 1;
    }
    printf("--- STEP 4: Window Created ---\n");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 6. Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    printf("--- STEP 5: ImGui Initialized. Entering Loop... ---\n");

    // 7. Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ---- DRAW GUI MANUALLY HERE (No separate functions to keep it simple) ----
        
        // Window 1: Controls
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Always);
        ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize);
        
        ImGui::InputText("Trace", trace_path, sizeof(trace_path));
        ImGui::RadioButton("First Fit", &fit_choice, 0); ImGui::SameLine();
        ImGui::RadioButton("Best Fit",  &fit_choice, 1); ImGui::SameLine();
        ImGui::RadioButton("Worst Fit", &fit_choice, 2);

        if (ImGui::Button("RUN", ImVec2(100, 40))) {
            printf("Button Pressed\n");
            
            sim::SimConfig cfg;
            cfg.trace_file = trace_path;
            // Map int to Enum
            if (fit_choice == 0) cfg.strategy = sim::FitStrategy::FirstFit;
            else if (fit_choice == 1) cfg.strategy = sim::FitStrategy::BestFit;
            else cfg.strategy = sim::FitStrategy::WorstFit;

            timeline.clear();
            timestamps.clear();
            
            try {
                if (sim::run_simulation(cfg, timeline)) {
                    for (auto const& [t, snap] : timeline.all()) {
                        timestamps.push_back(t);
                    }
                    simulation_ran = true;
                    printf("Simulation Success: %zu steps\n", timestamps.size());
                }
            } catch (std::exception& e) {
                printf("Error: %s\n", e.what());
            }
        }
        ImGui::End();

        // Window 2: Results
        ImGui::SetNextWindowPos(ImVec2(440, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Always);
        ImGui::Begin("Results", nullptr, ImGuiWindowFlags_NoResize);
        if (simulation_ran) {
            ImGui::Text("Steps: %zu", timestamps.size());
            if (!timestamps.empty()) {
                 if (ImGui::BeginListBox("Timestamps")) {
                    for (int i=0; i<timestamps.size(); i++) {
                        char buf[32];
                        sprintf(buf, "%llu", (unsigned long long)timestamps[i]);
                        if(ImGui::Selectable(buf, selected_timestamp_idx == i)) 
                            selected_timestamp_idx = i;
                    }
                    ImGui::EndListBox();
                 }
            }
        } else {
            ImGui::Text("Press Run...");
        }
        ImGui::End();
        // -------------------------------------------------------------------------

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
