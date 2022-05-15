#include "ControlView.h"
#include "imgui/imgui.h"
#include "imgui/ImGuiFileDialog.h"


struct ControlView::Impl {
    float progress{0};
    SP<AVPlayer> player;

    void draw();
};

void ControlView::Impl::draw() {

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_None);
    ImGui::SetNextWindowSize(ImVec2(1280, 60), ImGuiCond_None);
    ImGui::Begin("navi", NULL, ImGuiWindowFlags_MenuBar |
                               ImGuiWindowFlags_NoTitleBar |
                               ImGuiWindowFlags_NoMove |
                               ImGuiWindowFlags_NoBackground |
                               ImGuiWindowFlags_NoDecoration);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open..", "Ctrl+O")) {
                ImGuiFileDialog::Instance()->OpenDialog("videoIn", "Choose Video File", ".mp4", ".");
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();

    // display
    if (ImGuiFileDialog::Instance()->Display("videoIn", ImGuiWindowFlags_NoCollapse, ImVec2(640, 360),
                                             ImVec2(1280, 720))) {
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
            // action
            player->OpenVideo(filePathName);
            player->Play();
        }

        // close
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SetNextWindowPos(ImVec2(0, 670), ImGuiCond_None);
    ImGui::SetNextWindowSize(ImVec2(1280, 50), ImGuiCond_None);
    ImGui::SetNextWindowBgAlpha(.8);
    ImGui::Begin("control_view", NULL, ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize);

    ImGui::SetCursorPos(ImVec2(80, 15));
    ImGui::PushItemWidth(1180);

    if (player != nullptr) {
        if (player->GetStat() >= PLAYER_STAT::INIT) {
            progress = player->GetProgress() / player->GetDuration();
        }
    }
    float new_progress = progress;
    ImGui::SliderFloat("", &new_progress, 0.0f, 1.0f, "");

    if (progress != new_progress) {
            if (player->GetStat() >= PLAYER_STAT::INIT) {
                player->Pause();
                auto ts = int64_t(new_progress * player->GetDuration() * 1000);
                player->Seek(ts);
                player->Pause();
            }
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::SetCursorPos(ImVec2(10, 10));
    if (ImGui::Button(player->GetStat() != PLAYER_STAT::PAUSE ? "play" : "pause", ImVec2(60, 30))) {
        if (player->GetStat() >= PLAYER_STAT::INIT) {
            player->Pause();
        }
    }

    ImGui::End();
}


void ControlView::SetPlayer(SP<AVPlayer> player) {
    _impl->player = player;
}

void ControlView::Draw() {
    _impl->draw();
}

ControlView::ControlView() : _impl(make_up<ControlView::Impl>()) {

}

ControlView::~ControlView() {

}
