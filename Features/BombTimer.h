#pragma once
#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>
#include <sstream>
#include <ctime>
#include <string>
#include "..\Game\Entity.h"
#include "..\Core\Config.h"

namespace bmb
{
	bool isPlanted = false;
	std::time_t plantTime;

	// Анимация
	static float windowAlpha = 0.0f;
	static float defuseTextAlpha = 0.0f;
	static float boomTextAlpha = 0.0f;
	static bool isAnimating = false;
	static bool showBoom = false;
	static uint64_t animationStartTime = 0;
	static uint64_t boomStartTime = 0;

	// Defuse tracking
	static bool isBeingDefused = false;
	static bool wasBeingDefused = false;
	static float defuseTimeRemaining = 0.0f;
	static uint64_t lastDefuseCheck = 0;

	uint64_t currentTimeMillis() {
		using namespace std::chrono;
		return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}

	void UpdateDefuseState(uintptr_t bomb, DWORD64 entityListEntry) {
		uint64_t currentTime = currentTimeMillis();

		// Check defuse state less frequently to avoid spam
		if (currentTime - lastDefuseCheck < 100) return; // Check every 100ms
		lastDefuseCheck = currentTime;

		bool currentlyBeingDefused = false;
		memoryManager.ReadMemory(bomb + Offset.C4.m_bBeingDefused, currentlyBeingDefused);

		// Detect defuse start
		if (currentlyBeingDefused && !wasBeingDefused) {
			isBeingDefused = true;
			wasBeingDefused = true;

			// Get defuse time (5 or 10 seconds)
			float defuseEndTime = 0.0f;
			if (memoryManager.ReadMemory<float>(bomb + Offset.C4.m_flDefuseCountDown, defuseEndTime) && defuseEndTime > 0.0f) {
				DWORD64 globalVars = 0;
				if (memoryManager.ReadMemory<DWORD64>(gGame.GetClientDLLAddress() + Offset.GlobalVars, globalVars) && globalVars) {
					globalvars gv{ globalVars };
					if (gv.GetcurrentTime() && gv.g_fCurrentTime > 0.0f) {
						defuseTimeRemaining = defuseEndTime - gv.g_fCurrentTime;
					}
				}
			}
		}
		// Detect defuse interruption
		else if (!currentlyBeingDefused && wasBeingDefused) {
			isBeingDefused = false;
			wasBeingDefused = false;
			defuseTimeRemaining = 0.0f;
		}
		// Update remaining defuse time
		else if (currentlyBeingDefused) {
			float defuseEndTime = 0.0f;
			if (memoryManager.ReadMemory<float>(bomb + Offset.C4.m_flDefuseCountDown, defuseEndTime) && defuseEndTime > 0.0f) {
				DWORD64 globalVars = 0;
				if (memoryManager.ReadMemory<DWORD64>(gGame.GetClientDLLAddress() + Offset.GlobalVars, globalVars) && globalVars) {
					globalvars gv{ globalVars };
					if (gv.GetcurrentTime() && gv.g_fCurrentTime > 0.0f) {
						defuseTimeRemaining = defuseEndTime - gv.g_fCurrentTime;
						if (defuseTimeRemaining < 0.0f) {
							isBeingDefused = false;
							wasBeingDefused = false;
							defuseTimeRemaining = 0.0f;
						}
					}
				}
			}
		}
	}

	void UpdateAnimation(float deltaTime) {
		const float animationSpeed = 3.0f; // Скорость анимации

		if (isPlanted && !isAnimating) {
			isAnimating = true;
			animationStartTime = currentTimeMillis();
		}

		if (!isPlanted) {
			isAnimating = false;
			windowAlpha = 0.0f;
			defuseTextAlpha = 0.0f;
			boomTextAlpha = 0.0f;
			showBoom = false;
			isBeingDefused = false;
			wasBeingDefused = false;
			defuseTimeRemaining = 0.0f;
			return;
		}

		if (isAnimating) {
			uint64_t currentTime = currentTimeMillis();
			float elapsed = (currentTime - animationStartTime) / 1000.0f;

			// Анимация появления окна
			windowAlpha = (elapsed * animationSpeed > 1.0f) ? 1.0f : elapsed * animationSpeed;
		}

		// Анимация BOOM! текста
		if (showBoom) {
			uint64_t currentTime = currentTimeMillis();
			float elapsed = (currentTime - boomStartTime) / 1000.0f;

			if (elapsed < 1.0f) { // Показывать BOOM! 1 секунду
				boomTextAlpha = (elapsed < 0.2f) ? elapsed * 5.0f : 1.0f; // Быстрое появление
			} else if (elapsed < 2.0f) { // Затем затухание
				boomTextAlpha = (2.0f - elapsed) * 1.0f;
			} else {
				// После показа BOOM! скрываем всё
				showBoom = false;
				isPlanted = false;
				isAnimating = false;
				windowAlpha = 0.0f;
				defuseTextAlpha = 0.0f;
				boomTextAlpha = 0.0f;
			}
		}
	}

	int getBombSite(bool Planted)
	{
		if (Planted)
		{
			int site;
			uintptr_t cPlantedC4;
			if (!memoryManager.ReadMemory<uintptr_t>(gGame.GetClientDLLAddress() + Offset.PlantedC4, cPlantedC4))
				return 0;
			if (!memoryManager.ReadMemory<uintptr_t>(cPlantedC4, cPlantedC4))
				return 0;
			if (!memoryManager.ReadMemory<int>(cPlantedC4 + Offset.C4.m_nBombSite, site))
				return 0;

			return site;
		}
		else
			return 0;
		
	}

	void RenderWindow(int inGame)
	{
		if ((!MiscCFG::bmbTimer) || (inGame == 0 && !MenuConfig::ShowMenu))
			return;

		// Обновляем анимацию
		UpdateAnimation(ImGui::GetIO().DeltaTime);

		uintptr_t bomb;
		bool isBombPlanted;
		ImColor color = MiscCFG::BombTimerCol;
		auto plantedAddress = gGame.GetClientDLLAddress() + Offset.PlantedC4;

		memoryManager.ReadMemory(plantedAddress, bomb);
		memoryManager.ReadMemory(bomb, bomb);
		memoryManager.ReadMemory(plantedAddress - 0x8, isBombPlanted);

		auto time = currentTimeMillis();

		if (isBombPlanted && !isPlanted && (plantTime == NULL || time - plantTime > 60000))
		{
			isPlanted = true;
			plantTime = time;
		}

		// Update defuse state
		UpdateDefuseState(bomb, gGame.GetEntityListEntry());

		// Показывать окно только когда бомба заложена и таймер активен, или когда меню открыто
		if (!isPlanted && !MenuConfig::ShowMenu)
			return;

		float remaining = (40000 - (int64_t)time + plantTime) / (float)1000;

		// Скрывать окно после взрыва/обезвреживания бомбы
		if (isPlanted && remaining <= 0 && !isBombPlanted)
		{
			isPlanted = false;
			if (!MenuConfig::ShowMenu)
				return;
		}

		static float windowWidth = 200.0f;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;

		// Добавляем скругление краев и прозрачность
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, windowAlpha);

		ImGui::SetNextWindowPos(MenuConfig::BombWinPos, ImGuiCond_Once);
		ImGui::SetNextWindowSize({ windowWidth, 0 }, ImGuiCond_Once);

		ImGui::Begin("Bomb Timer", nullptr, flags);

		if (MenuConfig::BombWinChengePos)
		{
			ImGui::SetWindowPos("Bomb Timer", MenuConfig::BombWinPos);
			MenuConfig::BombWinChengePos = false;
		}

		float startPosX = ((ImGui::GetWindowSize().x - 180) * 0.5f) + 3;

		if (isPlanted && remaining >= 0)
		{
			float displayTime = 0.0f;
			float maxTime = 0.0f;
			std::string timerType = "";

			if (isBeingDefused && defuseTimeRemaining > 0.0f)
			{
				// Show defuse timer
				displayTime = defuseTimeRemaining;
				maxTime = 10.0f; // Maximum defuse time
				timerType = "DEFUSING";
				color = ImColor(32, 178, 170); // Cyan for defuse
			}
			else
			{
				// Show bomb timer
				displayTime = remaining;
				maxTime = 40.0f; // Bomb timer
				timerType = "Bomb on " + std::string(!getBombSite(isBombPlanted) ? "A" : "B");

				if (remaining <= 10)
				{
					color = ImColor(160, 48, 73); // Red for critical time
				}
				else
				{
					color = MiscCFG::BombTimerCol; // Normal color
				}
			}

			float barLength = displayTime <= 0.0f ? 0.0f : displayTime >= maxTime ? 1.0f : (displayTime / maxTime);

			std::ostringstream ss;
			ss.precision(3);
			ss << timerType << ": " << std::fixed << std::round(displayTime * 1000.0) / 1000.0 << " s";
			Gui.MyText(std::move(ss).str().c_str(), true);

			Gui.MyProgressBar(barLength, { 180, 15 }, "", color);
		}
		else
		{
			Gui.MyText("C4 not planted", true);
			Gui.MyProgressBar(0.0f, { 180, 15 }, "", color);
		}

		// Показываем BOOM! при взрыве
		if (showBoom && boomTextAlpha > 0.0f) {
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
			ImGui::SetCursorPosX(startPosX);
			ImColor boomColor = ImColor(255, 0, 0, static_cast<int>(255 * boomTextAlpha));
			ImGui::TextColored(boomColor, "BOOM!");
		}

		// Проверяем окончание раунда (когда бомба больше не planted, но таймер еще идет)
		if (isPlanted && !isBombPlanted && remaining > 0) {
			// Раунд закончился до взрыва - скрываем таймер
			isPlanted = false;
			isAnimating = false;
			windowAlpha = 0.0f;
			if (!MenuConfig::ShowMenu) {
				return;
			}
		}

		// Проверяем взрыв бомбы
		if (isPlanted && remaining <= 0 && !showBoom) {
			showBoom = true;
			boomStartTime = currentTimeMillis();
		}
		// Сбрасываем состояние после взрыва или обезвреживания
		if (isPlanted && !isBombPlanted && remaining <= 0)
		{
			isPlanted = false;
		}
		MenuConfig::BombWinPos = ImGui::GetWindowPos();
		ImGui::End();

		// Восстанавливаем стили
		ImGui::PopStyleVar(3);
	}
}