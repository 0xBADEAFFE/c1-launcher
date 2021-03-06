#include <cstring>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Util.h"
#include "DLL.h"

std::string Util::NumberToString(size_t number, int base, bool upperCase)
{
	if (base < 2 || base > 36)
	{
		return std::string();
	}

	const char firstLetter = (upperCase) ? 'A' : 'a';

	std::string result;

	do
	{
		const int digit = number % base;
		const char ch = (digit < 10) ? '0' + digit : firstLetter + (digit - 10);

		result += ch;

		number /= base;
	}
	while (number);

	std::reverse(result.begin(), result.end());

	return result;
}

const char *Util::GetCmdLine()
{
	return GetCommandLineA();
}

/**
 * @brief Obtains game version from any Crysis DLL.
 * It parses version resource of the specified file.
 * @param dll Any loaded Crysis DLL.
 * @return Game build number or 0 if some error occurred.
 */
int Util::GetCrysisGameBuild(const DLL & dll)
{
	HMODULE module = static_cast<HMODULE>(dll.getHandle());
	if (!module)
		return 0;

	// VERSIONINFO resource always has ID 1
	HRSRC versionResInfo = FindResource(module, MAKEINTRESOURCE(1), RT_VERSION);
	if (!versionResInfo)
		return 0;

	HGLOBAL versionResData = LoadResource(module, versionResInfo);
	if (!versionResData)
		return 0;

	void *versionRes = LockResource(versionResData);  // this function does nothing
	if (!versionRes)
		return 0;

	if (std::memcmp(RVA(versionRes, 0x6), L"VS_VERSION_INFO", 0x20) != 0)
		return 0;

	VS_FIXEDFILEINFO *pFileInfo = static_cast<VS_FIXEDFILEINFO*>(RVA(versionRes, 0x6 + 0x20 + 0x2));
	if (pFileInfo->dwSignature != 0xFEEF04BD)
		return 0;

	return LOWORD(pFileInfo->dwFileVersionLS);
}

/**
 * @brief Shows message box with error message and blocks until user closes it.
 * It also shows current last-error code and its description.
 * @param msg The error message to show.
 */
void Util::ErrorBox(const char *msg)
{
	DWORD errorCode = GetLastError();

	std::string result;

	result += msg;
	result += '\n';

	if (errorCode)
	{
		result += "Error";
		result += ' ';
		result += Util::NumberToString(errorCode);

		char errorMsgBuffer[512];
		if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
		                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMsgBuffer, sizeof errorMsgBuffer, NULL))
		{
			result += ": ";
			result += errorMsgBuffer;
		}
	}

	MessageBoxA(NULL, result.c_str(), "Error", MB_OK | MB_ICONERROR);
}

/**
 * @brief Fills read-only memory region with x86 NOP instruction.
 * @param address Address of the memory region.
 * @param length Size of the memory region in bytes.
 * @return True if no error occurred, otherwise false.
 */
bool Util::FillNOP(void *address, size_t length)
{
	DWORD oldProtection;

	if (VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &oldProtection) == 0)
		return false;

	// 0x90 is opcode of NOP instruction on both x86 and x86-64
	std::memset(address, '\x90', length);

	if (VirtualProtect(address, length, oldProtection, &oldProtection) == 0)
		return false;

	return true;
}

/**
 * @brief Fills read-only memory region with custom data.
 * The memory region and the data must not overlap.
 * @param address Address of the memory region.
 * @param data Address of the data.
 * @param length Size of the data in bytes.
 * @return True if no error occurred, otherwise false.
 */
bool Util::FillMem(void *address, const void *data, size_t length)
{
	DWORD oldProtection;

	if (VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &oldProtection) == 0)
		return false;

	std::memcpy(address, data, length);

	if (VirtualProtect(address, length, oldProtection, &oldProtection) == 0)
		return false;

	return true;
}
