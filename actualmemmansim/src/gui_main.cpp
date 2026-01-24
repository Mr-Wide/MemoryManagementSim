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
static char trace_path[256] = "trace.csv";

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
        timeline.clear();
        timestamps.clear();
        selected_timestamp = -1;

        SimConfig cfg;
        cfg.trace_file = trace_path;
        cfg.strategy =
            fit_choice == 0 ? FitStrategy::FirstFit :
            fit_choice == 1 ? FitStrategy::BestFit  :
                              FitStrategy::WorstFit;

        simulation_ran = run_simulation(cfg, timeline);

        // Fill timestamps
        for (auto &[time, _] : timeline.all())
            timestamps.push_back(time);

        std::cout << "Simulation finished. Available timestamps:\n";
        for (auto t : timestamps)
            std::cout << "  " << t << "\n";
    }

    ImGui::End();
}

// ---- TIMELINE PANEL ----
void draw_timeline() {
    ImGui::Begin("Timeline");

    if (!simulation_ran) {
        ImGui::Text("Run a simulation first.");
        ImGui::End();
        return;
    }

    ImGui::Text("Available timestamps:");

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

    ImGui::BeginChild("Events", ImVec2(0, 0), true);
    if (selected_timestamp >= 0 && selected_timestamp < timestamps.size()) {
        uint64_t t = timestamps[selected_timestamp];
        auto snap_opt = timeline.get(t); // timeline.get(time) returns optional snapshot
        if (snap_opt) {
            for (auto &e : snap_opt->events)
                ImGui::BulletText("PID %u: %s", e.pid, e.message.c_str());
        } else {
            ImGui::Text("No events at this timestamp.");
        }
    } else {
        ImGui::Text("Select a timestamp to see events.");
    }
    ImGui::EndChild();

    ImGui::End();
}

// ---- MAIN ----
int main() {
    if (!glfwInit()) return 1;

    GLFWwindow* window =
        glfwCreateWindow(1200, 800, "Memory Simulator (GUI)", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_control_panel();
        draw_timeline();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}
