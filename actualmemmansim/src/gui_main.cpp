#include "sim/simrunner.h"
#include "sim/timeline.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iostream>

using namespace sim;

static Timeline timeline;
static bool simulation_ran = false;
static int fit_choice = 0;
static char trace_path[256] = "trace.csv";

void draw_control_panel() {
    ImGui::Begin("Simulation Control");

    ImGui::InputText("Trace file", trace_path, sizeof(trace_path));

    ImGui::RadioButton("First Fit", &fit_choice, 0);
    ImGui::RadioButton("Best Fit",  &fit_choice, 1);
    ImGui::RadioButton("Worst Fit", &fit_choice, 2);

    if (ImGui::Button("Run Simulation")) {
        timeline.clear();

        SimConfig cfg;
        cfg.trace_file = trace_path;
        cfg.strategy =
            fit_choice == 0 ? FitStrategy::FirstFit :
            fit_choice == 1 ? FitStrategy::BestFit  :
                              FitStrategy::WorstFit;

        simulation_ran = run_simulation(cfg, timeline);
    }

    ImGui::End();
}

void draw_timeline() {
    ImGui::Begin("Timeline");

    for (auto &[time, snap] : timeline.all()) {
        if (ImGui::TreeNode((void*)(intptr_t)time, "Time %lu", time)) {
            for (auto &e : snap.events)
                ImGui::BulletText("PID %u: %s",
                                  e.pid, e.message.c_str());
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

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
        if (simulation_ran)
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
