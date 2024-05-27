#pragma once
#include <filesystem>
#include <mini/ini.h>

#include "MMU.h"
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "inputManager.h"
#include "serialPort.h"
#include "Cartridge.h"
#include "stringUtils.h"

enum class FileLoadResult
{
	Success,
	InvalidROM,
	SaveStateROMNotFound
};

class GBCore
{
public:
	static constexpr int32_t CYCLES_PER_FRAME = 17556;
	static constexpr double FRAME_RATE = 1.0 / 59.7;

	static constexpr int32_t calculateCycles(double deltaTime) { return static_cast<int>((CYCLES_PER_FRAME * (deltaTime / FRAME_RATE))); }

	void update(int32_t cyclesToExecute = CYCLES_PER_FRAME);
	void stepComponents();

	#ifdef  _WIN32

	inline FileLoadResult loadFile(const wchar_t* _filePath)
	{
		std::ifstream st(_filePath, std::ios::in | std::ios::binary);
		this->filePath = StringUtils::ToUTF8(_filePath);
		return loadFile(st);
	}

	#else

	inline FileLoadResult loadFile(const char* _filePath)
	{
		std::ifstream st(_filePath, std::ios::in | std::ios::binary);
		this->filePath = _filePath;
		return loadFile(st);
	}

	#endif

	inline void loadState(int num)
	{
		if (!cartridge.ROMLoaded) return;

		if (currentSave != 0)
			saveState(currentSave);

		const auto saveName = "/save" + std::to_string(num);
		const auto _filePath = StringUtils::nativePath(saveFolderPath + saveName + ".mbs");

		std::ifstream st(_filePath, std::ios::in | std::ios::binary);

		if (st && isSaveStateFile(st))
		{
			loadState(st);
			currentSaveName = saveName;
			currentSave = num;
		}
	}

	inline void saveState(int num)
	{
		if (!cartridge.ROMLoaded) return;

		const auto saveName = "/save" + std::to_string(num);
		const auto _filePath = StringUtils::nativePath(saveFolderPath + saveName + ".mbs");

		if (currentSave != num && std::filesystem::exists(_filePath))
			backupSave(num);

		saveState(_filePath);

		if (currentSave == 0)
		{ 
			currentSaveName = saveName;
			currentSave = num;
		}
	}

	constexpr int getSaveNum() { return currentSave; }
	constexpr auto& getSaveFolderPath() { return saveFolderPath; }

	template <typename T>
	inline void saveState(T filePath)
	{
		if (!cartridge.ROMLoaded || cpu.isExecutingBootROM())
			return;

		std::ofstream st(filePath, std::ios::out | std::ios::binary);
		saveState(st);
	}

	template <typename T>
	inline void saveBattery(T filePath)
	{
		std::ofstream st(filePath, std::ios::out | std::ios::binary);
		cartridge.getMapper()->saveBattery(st);
	}

	inline void loadBattery()
	{
		if (!cartridge.ROMLoaded || !cartridge.hasBattery) return;
		saveCurrentROM();
		restartROM(false);
	}

	inline void saveCurrentROM()
	{
		autoSave();
		backupSave(currentSave);
		batteryAutoSave();
	}

	void autoSave();
	void backupSave(int num);

	void batteryAutoSave();

	void reset();
	void restartROM(bool resetBattery = true);

	bool emulationPaused { false };
;	std::string gameTitle { };

	MMU mmu { *this };
	CPU cpu { *this };
	PPU ppu{ mmu, cpu };
	APU apu{};
	inputManager input { cpu };
	serialPort serial { cpu };
	Cartridge cartridge { *this };
private:
	std::string saveFolderPath;
	std::string filePath;
	std::string romFilePath;

	std::string currentSaveName {""};
	int currentSave {0};

	static constexpr std::string_view SAVE_STATE_SIGNATURE = "MegaBoy Emulator Save State";
	bool isSaveStateFile(std::ifstream& st);

	void saveState(std::ofstream& st);
	bool loadState(std::ifstream& st);
	FileLoadResult loadFile(std::ifstream& st);

	void loadBootROM();
};