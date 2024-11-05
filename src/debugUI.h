#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "GBCore.h"
#include "Utils/glFunctions.h"
#include "Utils/pixelOps.h"

extern GBCore gbCore;

class debugUI
{
public:
	static void updateMenu();
	static void updateWindows(float scaleFactor);

	static void signalVBlank();
	static void signalROMLoaded();
	static void signalBreakpoint();
private:
	enum class VRAMTab 
	{
		TileData,
		BackgroundMap,
		WindowMap,
		OAM
	};
	struct instructionHistoryEntry
	{
		uint16_t addr;
		uint8_t length;
		std::array<uint8_t, 3> data;
		std::string disasm;

		bool operator < (uint16_t addr) const
		{
			return this->addr < addr;
		}
	};

	static inline bool showMemoryView { false };
	static inline bool showCPUView { false };
	static inline bool showDisassembly { false };
	static inline bool showAudioView{ false };

	static inline bool showVRAMView{ false };
	static inline VRAMTab currentTab = VRAMTab::TileData;

	static inline std::unique_ptr<uint8_t[]> BGFrameBuffer;
	static inline std::unique_ptr<uint8_t[]> windowFrameBuffer;
	static inline std::unique_ptr<uint8_t[]> tileDataFrameBuffer;

	static inline uint32_t backgroundTexture {0};
	static inline uint32_t windowTexture {0};
	static inline uint32_t tileDataTexture {0};
	static inline uint32_t oamTexture {0};

	static inline int vramTileBank{ 0 };

	static inline int romMemoryView{ false };
	static inline int romDisassemblyView{ false };

	static inline int dissasmRomBank{ 0 };
	static inline int memoryRomBank{ 0 };

	static inline std::vector<uint16_t> breakpoints{};
	static inline std::vector<std::string> romDisassembly;
	static inline std::vector<instructionHistoryEntry> breakpointDisassembly;

	static inline int breakpointDisasmLine { 0 };
	static inline bool showBreakpointHitWindow { false };
	static inline bool shouldScrollToPC{ false };
	static inline int32_t tempBreakpointAddr { -1 };
	static inline int32_t stepOutStartSPVal { -1 };

	static inline void setTempBreakpoint(uint16_t addr)
	{
		tempBreakpointAddr = addr;
		gbCore.breakpoints[addr] = true;
	}

	static inline void disassembleRom();
	static inline void removeTempBreakpoint();
	static inline void extendBreakpointDisasmWindow();

	static inline void clearBuffer(uint8_t* buffer, uint16_t width, uint16_t height) { PixelOps::clearBuffer(buffer, width, height, PPU::ColorPalette[0]); }
};