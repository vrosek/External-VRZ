#include "ConfigMenu.h"
#include "../Core/Config.h"
#include "ConfigSaver.h"
#include <filesystem>
#include <string>
#include "../Resources/Language.h"
#include "../Helpers/KeyManager.h"

namespace ConfigMenu {
	
	void RenderCFGmenu()
	{
		ImGui::Columns(2, nullptr, false);
		static char configNameBuffer[128] = "NewConfig";
		static char configAuthorBuffer[128] = "Author";
		static int selectedConfig = -1;

		const std::string configDir = MenuConfig::path;
		static std::vector<std::string> configFiles;
		std::vector<const char*> configFilesCStr;

		configFiles.clear();
		for (const auto& entry : std::filesystem::directory_iterator(configDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".cfg")
			{
				configFiles.push_back(entry.path().filename().string());
			}
		}
		for (const auto& file : configFiles)
		{
			configFilesCStr.push_back(file.c_str());
		}

		float CursorX = 10.f;
		float CurrentCursorX = ImGui::GetCursorPosX();
		float ComponentWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x - CursorX * 2;

		ImGui::SetCursorPosX(20.f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 0.9f));
		ImGui::Text("CONFIG LOADER");
		ImGui::PopStyleColor();
		ImGui::Spacing();

		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::TextDisabled(Text::Config::MyConfigs.c_str());
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		ImGui::SetNextItemWidth(ComponentWidth);
		ImGui::ListBox("##ConfigFiles", &selectedConfig, configFilesCStr.data(), configFilesCStr.size());
		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.20f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.25f, 1.0f));
		if (ImGui::Button(Text::Config::Load.c_str(), { 120.f, 32.f }) && selectedConfig >= 0 && selectedConfig < configFiles.size())
		{
			std::string selectedConfigFile = configFiles[selectedConfig];
			MyConfigSaver::LoadConfig(selectedConfigFile);
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		
		ImGui::SameLine();
		
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.65f, 1.0f, 1.0f));
		if (ImGui::Button(Text::Config::Save.c_str(), { 120.f, 32.f }) && selectedConfig >= 0 && selectedConfig < configFiles.size())
		{
			std::string selectedConfigFile = configFiles[selectedConfig];
			MyConfigSaver::SaveConfig(selectedConfigFile);
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		ImGui::SetCursorPosX(CurrentCursorX + CursorX);
		
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		if (ImGui::Button(Text::Config::Delete.c_str(), { 120.f, 32.f }) && selectedConfig >= 0 && selectedConfig < configFiles.size())
			ImGui::OpenPopup("##reallyDelete");
		if (ImGui::BeginPopup("##reallyDelete"))
		{
			ImGui::Text("   Are you sure?   ");
			ImGui::Text(" ");
			ImGui::SameLine();
			if (ImGui::Button("No", { 40.0f, 0.0f }))
				ImGui::CloseCurrentPopup();
			ImGui::SameLine();
			if (ImGui::Button("Yes", { 40.0f, 0.0f }))
			{
				// Delete
				std::string selectedConfigFile = configFiles[selectedConfig];
				std::string fullPath = configDir + "\\" + selectedConfigFile;
				if (std::remove(fullPath.c_str()) == 0)
				{
					configFiles.erase(configFiles.begin() + selectedConfig);
					selectedConfig = -1;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		
		ImGui::SameLine();
		
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.0f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.5f, 0.0f, 1.0f));
		if (ImGui::Button(Text::Config::Reset.c_str(), { 120.f, 32.f }))
			ImGui::OpenPopup("##reallyReset");
		if (ImGui::BeginPopup("##reallyReset"))
		{
			ImGui::Text("   Are you sure?   ");
			ImGui::Text(" ");
			ImGui::SameLine();
			if (ImGui::Button("No", { 40.0f, 0.0f }))
				ImGui::CloseCurrentPopup();
			ImGui::SameLine();
			if (ImGui::Button("Yes", { 40.0f, 0.0f }))
			{
				ConfigMenu::ResetToDefault();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		ImGui::NextColumn();
		CurrentCursorX = ImGui::GetCursorPosX();
		ImGui::SetCursorPosX(CurrentCursorX + 10.f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 0.9f));
		ImGui::Text("CREATE CONFIG");
		ImGui::PopStyleColor();
		ImGui::Spacing();
		ImGui::TextDisabled(Text::Config::ConfigName.c_str());
		ImGui::SetNextItemWidth(ComponentWidth + 8);
		ImGui::InputText("###ConfigNameInput", configNameBuffer, sizeof(configNameBuffer));
		ImGui::TextDisabled(Text::Config::AuthorName.c_str());
		ImGui::SetNextItemWidth(ComponentWidth + 8);
		ImGui::InputText("###AuthorNameInput", configAuthorBuffer, sizeof(configAuthorBuffer));
		ImGui::NewLine();
		
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.65f, 0.20f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.75f, 0.25f, 1.0f));
		if (ImGui::Button(Text::Config::Create.c_str(), { 120.f, 32.f }))
		{
			std::string configFileName = std::string(configNameBuffer) + ".cfg";
			MyConfigSaver::SaveConfig(configFileName, std::string(configAuthorBuffer));
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		
		ImGui::SameLine();
		
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.65f, 1.0f, 1.0f));
		if (ImGui::Button(Text::Config::OpenFolder.c_str(), { 120.f, 32.f }))
		{
			Gui.OpenWebpage(configDir.c_str());
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		ImGui::Columns(1);
	}

	void ResetToDefault() {
		
		ESPConfig::ESPenabled = true;
		ESPConfig::ShowBoxESP = true;
		ESPConfig::BoxType = 1;
		ESPConfig::BoxRounding = 0.0f;
		ESPConfig::BoxColor = ImColor(255, 255, 255, 255);
		ESPConfig::OutLine = false;
		
		ESPConfig::ShowHeadBox = true;
		ESPConfig::HeadBoxColor = ImColor(131, 137, 150, 180);
		
		ESPConfig::ShowBoneESP = true;
		ESPConfig::BoneColor = ImColor(255, 255, 255, 255);
		
		ESPConfig::ShowPlayerName = true;
		ESPConfig::ShowHealthBar = true;
		ESPConfig::ShowHealthNum = true;
		ESPConfig::ShowWeaponESP = false;
		ESPConfig::ShowIsScoped = true;
		ESPConfig::ShowIsBlind = true;
		
		ESPConfig::ShowOutOfFOVArrow = false;
		ESPConfig::OutOfFOVRadiusFactor = 0.45f;
		
		ESPConfig::FlashCheck = false;
		ESPConfig::VisibleCheck = false;
		ESPConfig::VisibleColor = ImColor(59, 71, 148, 180);
		
		ESPConfig::ShowEyeRay = false;
		ESPConfig::ShowLineToEnemy = false;
		ESPConfig::HotKey = VK_F5;  // 117 = F6
		
		
		MiscCFG::BombTimerCol = ImColor(131, 137, 150, 255);
		MiscCFG::bmbTimer = false;
		MiscCFG::FastStop = false;
		MiscCFG::SpecList = false;
		MiscCFG::WaterMark = false;
		MiscCFG::BunnyHop = false;
		MiscCFG::HitSound = 0;
		MiscCFG::HitMarker = false;
		MiscCFG::SniperCrosshair = false;
		MiscCFG::AutoKnife = false;
		MiscCFG::AutoZeus = false;
		MiscCFG::AntiAFKKick = false;
		MiscCFG::ShowHeadShootLine = false;
		MiscCFG::AutoAccept = false;
		
		// Menu Config
		MenuConfig::ShowMenu = true;
		MenuConfig::WorkInSpec = true;
		MenuConfig::TeamCheck = true;
		MenuConfig::BypassOBS = false;
		MenuConfig::HotKey = VK_DELETE;  // 46 = Delete
		
		// Radar Config
		RadarCFG::ShowRadar = false;
		RadarCFG::RadarRange = 125;
		RadarCFG::ShowRadarCrossLine = false;
		RadarCFG::RadarType = 2;
		RadarCFG::RadarPointSizeProportion = 1.0f;
		RadarCFG::RadarBgAlpha = 0.1f;
		RadarCFG::Proportion = 2700.f;
		
		// Window Positions
		MenuConfig::MarkWinPos = ImVec2(1601.0f, 320.0f);
		MenuConfig::RadarWinPos = ImVec2(25.f, 25.f);
		MenuConfig::SpecWinPos = ImVec2(10.0f, 340.0f);
		MenuConfig::BombWinPos = ImVec2(860.0f, 80.0f);
		
		MenuConfig::MarkWinChengePos = true;
		MenuConfig::BombWinChengePos = true;
		MenuConfig::RadarWinChengePos = true;
		MenuConfig::SpecWinChengePos = true;

	}
}
