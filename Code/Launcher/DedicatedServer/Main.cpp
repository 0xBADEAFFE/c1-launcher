/**
 * @file
 * @brief Dedicated server launcher.
 */

#include <cstdlib>

#include "Launcher/Launcher.h"
#include "Launcher/DLL.h"
#include "Launcher/CPU.h"
#include "Launcher/Patch.h"
#include "Launcher/Util.h"

struct CrysisLibs
{
	DLL CryGame;
	DLL CryNetwork;
	DLL CrySystem;

	bool load()
	{
		// some Crysis DLLs can cause crash when being unloaded
		// so NO_RELEASE flag is used here to disable explicit unloading of the DLLs

		if (!CryGame.load("CryGame.dll", DLL::NO_RELEASE))
		{
			Util::ErrorBox("Failed to load the CryGame DLL!");
			return false;
		}

		if (!CrySystem.load("CrySystem.dll", DLL::NO_RELEASE))
		{
			Util::ErrorBox("Failed to load the CrySystem DLL!");
			return false;
		}

		if (!CryNetwork.load("CryNetwork.dll", DLL::NO_RELEASE))
		{
			Util::ErrorBox("Failed to load the CryNetwork DLL!");
			return false;
		}

		return true;
	}
};

static bool InstallMemoryPatches(const CrysisLibs & libs, int gameVersion)
{
	// CryNetwork
	{
		void *pCryNetwork = libs.CryNetwork.getHandle();

		if (!Patch::EnablePreordered(pCryNetwork, gameVersion))
			return false;

		if (!Patch::AllowSameCDKeys(pCryNetwork, gameVersion))
			return false;
	}

	// CrySystem
	{
		void *pCrySystem = libs.CrySystem.getHandle();

		if (!Patch::UnhandledExceptions(pCrySystem, gameVersion))
			return false;

		if (CPU::IsAMD() && !CPU::Has3DNow())
		{
			// Dedicated server usually doesn't execute any code with 3DNow! instructions.
			// However, we should still make sure that ISystem::GetCPUFlags always returns correct flags.

			if (!Patch::Disable3DNow(pCrySystem, gameVersion))
				return false;
		}
	}

	return true;
}

int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *lpCmdLine, int nCmdShow)
{
	CrysisLibs libs;

	if (!libs.load())
	{
		return EXIT_FAILURE;
	}

	const int gameVersion = Util::GetCrysisGameBuild(libs.CrySystem);
	if (!gameVersion)
	{
		Util::ErrorBox("Failed to obtain game version!");
		return EXIT_FAILURE;
	}

	// make sure all versions listed here are also supported in Patch.cpp
	switch (gameVersion)
	{
		// Crysis
		case 5767:
		case 5879:
		case 6115:
		case 6156:
		// Crysis Wars
		case 6729:
		{
			if (!InstallMemoryPatches(libs, gameVersion))
			{
				Util::ErrorBox("Failed to apply memory patch!");
				return EXIT_FAILURE;
			}

			break;
		}
		default:
		{
			Util::ErrorBox("Unsupported version of the game!");
			return EXIT_FAILURE;
		}
	}

	Launcher launcher;

	launcher.setAppInstance(hInstance);
	launcher.setLogFileName("Server.log");
	launcher.setDedicatedServer(true);

	return launcher.run(libs.CryGame) ? EXIT_SUCCESS : EXIT_FAILURE;
}
