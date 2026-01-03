#pragma once
#include "..\Core\Config.h"
#include "..\Core\Render.h"
#include "..\Features\Radar.h"
#include "..\Features\Misc.h"
#include "..\Config\ConfigMenu.h"
#include "..\Config\ConfigSaver.h"

#include "..\Resources\Language.h"
#include "..\Resources\Images.h"
#include "../Helpers/KeyManager.h"

#include "../Features/ESP.h"

namespace GUI
{
	void LoadDefaultConfig()
	{
		if (!MenuConfig::defaultConfig)
			return;

		// First, reset to defaults
		ConfigMenu::ResetToDefault();
		
		// Then try to load default.cfg if it exists
		MyConfigSaver::LoadConfig("default.cfg");

		MenuConfig::defaultConfig = false;
	}

	void LoadImages()
	{
		// Removed image loading as per user request
	}

	// Components Settings
	// ########################################
	void AlignRight(float ContentWidth)
	{
		float ColumnContentWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x;
		float checkboxPosX = ImGui::GetColumnOffset() + ColumnContentWidth - ContentWidth;
		ImGui::SetCursorPosX(checkboxPosX);
	}
	void PutSwitch(const char* string, float CursorX, float ContentWidth, bool* v, bool ColorEditor = false, const char* lable = NULL, float col[4] = NULL, const char* Tip = NULL)
	{
		ImGui::PushID(string);
		float CurrentCursorX = ImGui::GetCursorPosX();
		float CurrentCursorY = ImGui::GetCursorPosY();
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::TextDisabled(string);
		if (Tip && ImGui::IsItemHovered())
			ImGui::SetTooltip(Tip);
		ImGui::SameLine();
		ImGui::SetCursorPosY(CurrentCursorY - 2);
		if (ColorEditor) {
			AlignRight(ContentWidth + ImGui::GetFrameHeight() +7);
			ImGui::ColorEdit4(lable, col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
			ImGui::SameLine();
		}
		else {
			AlignRight(ContentWidth);
		}
		
		Gui.SwitchButton(string, v);
		ImGui::PopID();
	}
	void PutColorEditor(const char* text, const char* lable, float CursorX, float ContentWidth, float col[4], const char* Tip = NULL)
	{
		ImGui::PushID(text);
		float CurrentCursorX = ImGui::GetCursorPosX();
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::TextDisabled(text);
		if (Tip && ImGui::IsItemHovered())
			ImGui::SetTooltip(Tip);
		ImGui::SameLine();
		AlignRight(ContentWidth + ImGui::GetFrameHeight() + 8);
		ImGui::ColorEdit4(lable, col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
		ImGui::PopID();
	}
	void PutSliderFloat(const char* string, float CursorX, float* v, const void* p_min, const void* p_max, const char* format, const char* Tip = NULL)
	{
		// if there is no fucking ID, all the sliders would be fucking forced to sync when you click on one of them ;3
		ImGui::PushID(string);
		float CurrentCursorX = ImGui::GetCursorPosX();
		float SliderWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x - CursorX - 15;
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::TextDisabled(string);
		if (Tip && ImGui::IsItemHovered())
			ImGui::SetTooltip(Tip);
		ImGui::SameLine();
		ImGui::TextDisabled(format, *v);
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::SetNextItemWidth(SliderWidth);
		Gui.SliderScalarEx2("", ImGuiDataType_Float, v, p_min, p_max, "", ImGuiSliderFlags_None);
		ImGui::PopID();
	}
	void PutSliderInt(const char* string, float CursorX, int* v, const void* p_min, const void* p_max, const char* format, const char* Tip = NULL)
	{
		ImGui::PushID(string);
		float CurrentCursorX = ImGui::GetCursorPosX();
		float SliderWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x - CursorX-15;
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::TextDisabled(string);
		if (Tip && ImGui::IsItemHovered())
			ImGui::SetTooltip(Tip);
		ImGui::SameLine();
		ImGui::TextDisabled(format, *v);
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::SetNextItemWidth(SliderWidth);
		Gui.SliderScalarEx2("", ImGuiDataType_Float, v, p_min, p_max, "", ImGuiSliderFlags_None);
		ImGui::PopID();
	}
	// ########################################

	void DrawGui()
	{
		LoadImages();

		ImColor BorderColor = ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border));

		char TempText[256];
		ImGuiWindowFlags Flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
		ImGui::SetNextWindowPos({ (ImGui::GetIO().DisplaySize.x - MenuConfig::WCS.MainWinSize.x) / 2.0f, (ImGui::GetIO().DisplaySize.y - MenuConfig::WCS.MainWinSize.y) / 2.0f }, ImGuiCond_Once);
		ImGui::SetNextWindowSize(MenuConfig::WCS.MainWinSize);
		ImGui::Begin("Vrosik.", nullptr, Flags);
		{
			ImGui::SetCursorPos(MenuConfig::WCS.ChildPos);
			
			ImGui::BeginChild("Page", MenuConfig::WCS.ChildSize, false, ImGuiWindowFlags_NoScrollbar);
			{
				// Header with gradient background
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.f);
				ImGui::SetCursorPosX(20.f);
				
				// Title
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 1.0f)); // Accent color
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); // Bold font if available
				ImGui::Text("Vrosik");
				ImGui::PopFont();
				ImGui::PopStyleColor();
				
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 80.f);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				ImGui::Text("v1.0");
				ImGui::PopStyleColor();
				
				ImGui::Spacing();
				ImGui::Spacing();
				
				// Colored separator
				ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.26f, 0.59f, 0.98f, 0.5f));
				ImGui::Separator();
				ImGui::PopStyleColor();
				
				ImGui::Spacing();
				ImGui::Spacing();
				
				// Tabs with rounded style
				static int activeTab = 0;
				ImGui::SetCursorPosX(20.f);
				
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.f, 10.f));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f); // Rounded corners
				
				// Active tab colors
				ImVec4 activeColor = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
				ImVec4 inactiveColor = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
				
				// Aimbot Tab
				ImGui::PushStyleColor(ImGuiCol_Button, activeTab == 0 ? activeColor : inactiveColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.65f, 1.0f, 1.0f));
				if (ImGui::Button("Aimbot", { 95.f, 36.f })) activeTab = 0;
				ImGui::PopStyleColor(2);

				ImGui::SameLine(0, 10.f);

				// ESP Tab
				ImGui::PushStyleColor(ImGuiCol_Button, activeTab == 1 ? activeColor : inactiveColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.65f, 1.0f, 1.0f));
				if (ImGui::Button("ESP", { 95.f, 36.f })) activeTab = 1;
				ImGui::PopStyleColor(2);

				ImGui::SameLine(0, 10.f);

				// Misc Tab
				ImGui::PushStyleColor(ImGuiCol_Button, activeTab == 2 ? activeColor : inactiveColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.65f, 1.0f, 1.0f));
				if (ImGui::Button("Misc", { 95.f, 36.f })) activeTab = 2;
				ImGui::PopStyleColor(2);

				ImGui::SameLine(0, 10.f);

				// Config Tab
				ImGui::PushStyleColor(ImGuiCol_Button, activeTab == 3 ? activeColor : inactiveColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.65f, 1.0f, 1.0f));
				if (ImGui::Button("Config", { 95.f, 36.f })) activeTab = 3;
				ImGui::PopStyleColor(2);
				
				ImGui::PopStyleVar(2);
				
				ImGui::Spacing();
				ImGui::Spacing();
				
				ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.26f, 0.59f, 0.98f, 0.3f));
				ImGui::Separator();
				ImGui::PopStyleColor();
				
				ImGui::Spacing();
				ImGui::Spacing();
				
				// Aimbot Tab
				if (activeTab == 0)
				{
					ImGui::SetCursorPosX(20.f);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 0.9f));
					ImGui::Text("AIMBOT SETTINGS");
					ImGui::PopStyleColor();
					ImGui::Spacing();

					// Future aimbot settings will go here
					ImGui::Text("Aimbot settings will be added here");
				}

				// ESP Tab
				else if (activeTab == 1)
				{
					static const float MinRounding = 0.f, MaxRounding = 10.f;
					static const float MinFovFactor = 0.f, MaxFovFactor = 1.f;

					ImGui::SetCursorPosX(20.f);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 0.9f));
					ImGui::Text("ESP");
					ImGui::PopStyleColor();
					ImGui::Spacing();

					ImGui::Columns(2, nullptr, false);

						PutSwitch(Text::ESP::Enable.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ESPenabled);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.f);
					ImGui::TextDisabled(Text::ESP::HotKeyList.c_str());
					ImGui::SameLine();
					AlignRight(70.f);
					if (ImGui::Button(Text::ESP::HotKey.c_str(), { 70.f, 25.f }))
					{
						std::thread([&]() {
							KeyMgr::GetPressedKey(ESPConfig::HotKey, &Text::ESP::HotKey);
							}).detach();
					}

					if (ESPConfig::ESPenabled)
					{

						PutSwitch(Text::ESP::Box.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowBoxESP, true, "###BoxCol", reinterpret_cast<float*>(&ESPConfig::BoxColor));
						if (ESPConfig::ShowBoxESP)
						{
							PutSwitch(Text::ESP::Outline.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::OutLine);
							ImGui::TextDisabled(Text::ESP::BoxType.c_str());
							ImGui::SameLine();
							AlignRight(160.f);
							ImGui::SetNextItemWidth(160.f);
							ImGui::Combo("###BoxType", &ESPConfig::BoxType, "Normal\0Corner\0");
							PutSliderFloat(Text::ESP::BoxRounding.c_str(), 10.f, &ESPConfig::BoxRounding, &MinRounding, &MaxRounding, "%.1f");
						}
						PutSwitch(Text::ESP::HeadBox.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowHeadBox, true, "###HeadBoxCol", reinterpret_cast<float*>(&ESPConfig::HeadBoxColor));
						PutSwitch(Text::ESP::Skeleton.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowBoneESP, true, "###BoneCol", reinterpret_cast<float*>(&ESPConfig::BoneColor));
						PutSwitch(Text::ESP::OutOfFOVArrow.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowOutOfFOVArrow, true, "###OutFOVCol", reinterpret_cast<float*>(&ESPConfig::OutOfFOVArrowColor));
						if(ESPConfig::ShowOutOfFOVArrow)
							PutSliderFloat(Text::ESP::OutOfFOVRadius.c_str(), .5f, &ESPConfig::OutOfFOVRadiusFactor, &MinFovFactor, &MaxFovFactor, "%.1f");

						PutSwitch(Text::ESP::HealthBar.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowHealthBar);
						PutSwitch(Text::ESP::Weapon.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowWeaponESP);
						PutSwitch(Text::ESP::PlayerName.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::ShowPlayerName);
						PutSwitch(Text::ESP::FlashCheck.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::FlashCheck);
						PutSwitch(Text::ESP::VisCheck.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &ESPConfig::VisibleCheck, true, "###VisibleCol", reinterpret_cast<float*>(&ESPConfig::VisibleColor));
					}
					ImGui::NewLine();

					ImGui::NextColumn();
					ImGui::Columns(1);
				}

				// Misc Settings Tab
				else if (activeTab == 2)
				{
					ImGui::Columns(2, nullptr, false);
					
					ImGui::SetCursorPosX(20.f);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 0.9f));
					ImGui::Text("MOVEMENT");
					ImGui::PopStyleColor();
					ImGui::Spacing();
					
					PutSwitch(Text::Misc::BunnyHop.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::BunnyHop, false, NULL, NULL, Text::Misc::InsecureTip.c_str());
					
					ImGui::Spacing();
					ImGui::SetCursorPosX(20.f);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 0.9f));
					ImGui::Text("AUTO");
					ImGui::PopStyleColor();
					ImGui::Spacing();
					
					PutSwitch("Knife bot", 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::AutoKnife);
					PutSwitch("Zeus bot", 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::AutoZeus);
					PutSwitch(Text::Misc::bmbTimer.c_str(), 10.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::bmbTimer);

					ImGui::NextColumn();
					float CurrentCursorX = ImGui::GetCursorPosX();
					ImGui::SetCursorPosX(CurrentCursorX + 10.f);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 0.9f));
					ImGui::Text("SETTINGS");
					ImGui::PopStyleColor();
					ImGui::Spacing();
					
					ImGui::SetCursorPosX(CurrentCursorX + 5.f);
					ImGui::TextDisabled(Text::Misc::MenuKey.c_str());
					ImGui::SameLine();
					AlignRight(85.f);
					
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.22f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
					if (ImGui::Button(Text::Misc::HotKey.c_str(), { 85.f, 28.f }))
					{
						std::thread([&]() {
							KeyMgr::GetPressedKey(MenuConfig::HotKey, &Text::Misc::HotKey);
							}).detach();
					}
					ImGui::PopStyleColor(2);
					ImGui::PopStyleVar();
					
					PutSwitch(Text::Misc::TeamCheck.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &MenuConfig::TeamCheck);
					PutSwitch(Text::Misc::AntiRecord.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &MenuConfig::BypassOBS);
					// Radar Hack disabled for external cheat compatibility
					// PutSwitch(Text::Misc::RadarHack.c_str(), 5.f, ImGui::GetFrameHeight() * 1.7, &MiscCFG::RadarHack);
					
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::SetCursorPosX(CurrentCursorX + 5.f);
					
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
					if (ImGui::Button("Unhook", { 140.f, 32.f }))
						Init::Client::Exit();
					ImGui::PopStyleColor(3);
					ImGui::PopStyleVar();

					ImGui::Columns(1);
				}

				// Config Tab
				else if (activeTab == 3)
				{
					ConfigMenu::RenderCFGmenu();
				}
				ImGui::NewLine();
			} ImGui::EndChild();
		} ImGui::End();

		LoadDefaultConfig();
	}
}

