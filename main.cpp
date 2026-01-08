#include "Core/Cheats.h"
#include "Offsets/Offsets.h"
#include "Resources/Language.h"
#include "Core/Init.h"
#include "Config/ConfigSaver.h"
#include "Config/ConfigMenu.h"
#include "Helpers/Logger.h"
#include "Helpers/UIAccess.h"
#include <filesystem>
#include <KnownFolders.h>
#include <ShlObj.h>

using namespace std;

namespace fs = filesystem;
string fileName;

void Cheat();
bool CheckArg(const int&, char**, const std::string&);

int main(int argc, char* argv[])
{
#ifndef DBDEBUG
	DWORD err = PrepareForUIAccess();
	if (err != ERROR_SUCCESS)
	{
		MessageBoxA(NULL, "Failed to elevate to UIAccess.", "Error", MB_OK);
		return -1;
	}
#endif

	Cheat();
	return 0;
}

bool CheckArg(const int& argc, char** argv, const std::string& value)
{
	for (size_t i = 0; i < argc; i++)
	{
		std::string arg = argv[i];
		if (arg == "--" + value || arg == "/" + value)
			return true;
	}
	return false;
}

void Cheat()
{
	ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL);
	SetConsoleTitle(L"CS2Cheats");
	int tryCount = 0;

	Log::Custom(R"LOGO(Vrosik. 	Any questions - t.me/brbrbrvrz)LOGO", 13);
	Log::Info(MenuConfig::name + " v" + MenuConfig::version + " by " + MenuConfig::author);

	if (!Init::Verify::CheckWindowVersion())
		Log::Warning("Your os is unsupported, bugs may occurred", true);

	Log::Info("Updating offsets...");
	try
	{
		Offset.UpdateOffsets();
		Log::PreviousLine();
		Log::Fine("Offsets updated");
	}
	catch (const std::exception& error)
	{
		Log::PreviousLine();
		Log::Warning(std::string("Failed to update offsets: ") + error.what(), true);
	}

	Log::Info("Waiting for CS2...");
	bool preStart = false;
	while (memoryManager.GetProcessID(L"cs2.exe") == 0)
	{
		preStart = true;
		Sleep(500);
	}
	if (preStart)
	{
		Log::PreviousLine();
		Log::Info("Connecting to CS2 (this may take some time)...");
		Sleep(23000);
	}

	Log::PreviousLine();
	Log::Fine("Connected to CS2");
	Log::Info("Linking to CS2...");

	if (!memoryManager.Attach(memoryManager.GetProcessID(L"cs2.exe")))
	{
		Log::PreviousLine();
		Log::Error("Failed to attach to the process");
	}

	if (!gGame.InitAddress())
	{
		Log::PreviousLine();
		Log::Error("Failed to Init Address");
	}

	g_globalVars = std::make_unique<globalvars>();
	if (!g_globalVars->UpdateGlobalvars()) {
		Log::PreviousLine();
		Log::Warning("Failed to update global variables - this may not be critical", false);
	}

	Log::PreviousLine();
	Log::Fine("Linked to CS2");

	char documentsPath[MAX_PATH];
	if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath) != S_OK)
		Log::Error("Failed to get the Documents folder path");

	MenuConfig::path = documentsPath;
	MenuConfig::docPath = documentsPath;
	MenuConfig::path += "\\CS2Cheats";
	try
	{
		if (fs::exists(MenuConfig::docPath + "\\Adobe Software Data"))
			fs::rename(MenuConfig::docPath + "\\Adobe Software Data", MenuConfig::path);
		if (fs::exists(MenuConfig::path))
			Log::Fine("Config folder connected: " + MenuConfig::path);
		else
		{
			if (fs::create_directory(MenuConfig::path))
				Log::Fine("Config folder connected: " + MenuConfig::path);
			else
				Log::Error("Failed to create the config directory");
		}
		if (fs::exists(MenuConfig::path + "\\default.cfg"))
			MenuConfig::defaultConfig = true;
		else
		{
			Log::Info("Creating default config...");
			ConfigMenu::ResetToDefault();
			MyConfigSaver::SaveConfig("default.cfg", "Vrosik.");
			MenuConfig::defaultConfig = true;
		}
	}
	catch (const std::exception& error)
	{
		Log::Error(error.what());
	}

	Misc::Layout = Misc::DetectKeyboardLayout();

	Log::Fine("External VRZ loaded");

#ifndef DBDEBUG
	Sleep(3000);
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

	try
	{
		Gui.AttachAnotherWindow("Counter-Strike 2", "SDL_app", Cheats::Run);
	}
	catch (std::exception& error)
	{
		Log::Error(error.what());
	}
}
