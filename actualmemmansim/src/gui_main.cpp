#include "sim/timeline.h"
#include "sim/metrics.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <iostream>

using namespace sim;

static Timeline timeline;   // loaded after simulation

void draw_timeline_window() {
    ImGui::Begin("Timeline");

    for (const auto &[time, snap] : timeline.all()) {
        if (ImGui::TreeNode((void*)(intptr_t)time, "Time %llu", time)) {

            ImGui::Text("Events:");
            for (const auto &e : snap.events) {
                ImGui::BulletText("pid %u: %s",
                                  e.pid,
                                  e.message.c_str());
            }

            ImGui::Separator();
            ImGui::Text("Metrics:");
            ImGui::Text("Allocated: %llu", snap.metrics.allocated_bytes);
            ImGui::Text("Free: %llu", snap.metrics.free_bytes);
            ImGui::Text("Largest Free: %llu", snap.metrics.largest_free);
            ImGui::Text("Internal Frag: %.2f", snap.metrics.internal_frag);
            ImGui::Text("External Frag: %.2f", snap.metrics.external_frag);
            ImGui::Text("Page Faults: %zu", snap.metrics.page_faults);

            ImGui::TreePop();
        }
    }

    ImGui::End();
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return 1;
    }

    GLFWwindow* window =
        glfwCreateWindow(1200, 800, "Memory Simulator", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // TODO: run your simulation here
    // run_simulation(timeline);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_timeline_window();

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
