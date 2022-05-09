#include "ControlView.h"
#include "imgui/imgui.h"



struct ControlView::Impl
{
	float progress{ 0 };
    SP<AVPlayer> player;

	void draw();
};

void ControlView::Impl::draw() {
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	//window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::Begin("My First Tool", NULL, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..", "Ctrl+O")) {
                player->OpenVideo("/Users/hecc/Documents/hecc/dev/avfun/resources/sanguo.mp4");
                player->Play();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(0, 620), ImGuiCond_None);
	ImGui::SetNextWindowSize(ImVec2(1280, 100), ImGuiCond_None);
	ImGui::Begin("control_view", NULL, window_flags);

	ImGui::SetCursorPosX(40);
	ImGui::PushItemWidth(1200);
	ImGui::SliderFloat("", &progress, 0.0f, 1.0f);
	ImGui::PopItemWidth();

	ImGui::NewLine();

	ImGui::SetCursorPosX(590);

    if (ImGui::Button("play/pause", ImVec2(80,40)))
    {
        if(player != nullptr)
        {
            player->Pause();
        }
    }

    if (ImGui::Button("seek", ImVec2(80,40)))
    {
        if(player != nullptr)
        {
            player->Seek(0);
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

ControlView::ControlView():_impl(make_up<ControlView::Impl>()) {
	
}

ControlView::~ControlView() {

}
