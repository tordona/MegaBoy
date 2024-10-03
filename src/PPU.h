#pragma once

#include <cstdint>
#include <array>
#include <fstream>

#include "defines.h"
#include "Utils/pixelOps.h"
#include "Utils/bitOps.h"

using color = PixelOps::color;

enum class PPUMode : uint8_t
{
	HBlank = 0,
	VBlank = 1,
	OAMSearch = 2,
	PixelTransfer = 3,
};

struct ObjFIFOEntry
{
	uint8_t color : 2  {};
	uint8_t palette : 3 {};
	bool bgPriority : 1 {};
	bool spritePriority : 1{};
};

enum class FetcherState : uint8_t
{
	FetchTileNo,
	FetchTileDataLow,
	FetchTileDataHigh,
	PushFIFO
};

template <typename T>
struct PixelFIFO
{
	uint8_t cycles{ 0 };
	FetcherState state{ FetcherState::FetchTileNo };
	std::array<T, 8> data{};

	uint16_t tileMap{};
	uint8_t tileLow{};
	uint8_t tileHigh{};

	inline void push(T ent)
	{
		data[back++] = ent;
		back &= 0x7;
		size++;
	}
	inline T pop()
	{
		T val = data[front++];
		front &= 0x7;
		size--;
		return val;
	}

	inline T& operator[](uint8_t ind)
	{
		return data[(front + ind) & 0x7];
	}

	inline bool full() { return size == 8; };
	inline bool empty() { return size == 0; };

	inline void clear() { front = 0; back = 0; size = 0; }

	inline void reset()
	{
		cycles = 0;
		state = FetcherState::FetchTileNo;
		clear();
	}

	inline void saveState(std::ofstream& st)
	{
		ST_WRITE(cycles);
		ST_WRITE(state);
		ST_WRITE(front);
		ST_WRITE(back);
		ST_WRITE(size);
		ST_WRITE_ARR(data);
	}
	inline void loadState(std::ifstream& st)
	{
		ST_READ(cycles);
		ST_READ(state);
		ST_READ(front);
		ST_READ(back);
		ST_READ(size);
		ST_READ_ARR(data);
	}
protected:
	uint8_t front{ 0 };
	uint8_t back{ 0 };
	uint8_t size{ 0 };
};

struct ObjPixelFIFO : PixelFIFO<ObjFIFOEntry>
{
	uint8_t objInd{ };

	inline void reset()
	{
		PixelFIFO::reset();
		objInd = 0;
	}

	inline void saveState(std::ofstream& st)
	{
		ST_WRITE(objInd);
		PixelFIFO::saveState(st);
	}
	inline void loadState(std::ifstream& st)
	{
		ST_READ(objInd);
		PixelFIFO::loadState(st);
	}
};

struct BGPixelFIFO : PixelFIFO<uint8_t>
{
	uint8_t fetchX{ 0 };
	bool newScanline{ true };
	bool fetchingWindow{ false };

	inline void reset()
	{
		PixelFIFO::reset();
		fetchX = 0;
		newScanline = true;
		fetchingWindow = false;
	}

	inline void saveState(std::ofstream& st)
	{
		ST_WRITE(fetchX);
		ST_WRITE(newScanline);
		ST_WRITE(fetchingWindow);
		PixelFIFO::saveState(st);
	}
	inline void loadState(std::ifstream& st)
	{
		ST_READ(fetchX);
		ST_READ(newScanline);
		ST_READ(fetchingWindow);
		PixelFIFO::loadState(st);
	}
};

struct OAMobject
{
	int16_t X{};
	int16_t Y{};
	uint16_t tileAddr{};
	uint8_t attributes{};
};

struct ppuState
{
	bool lycFlag{ false };
	bool blockStat{ false };

	uint8_t LY{ 0 };
	uint8_t WLY{ 0 };

	int8_t scanlineDiscardPixels{ -1 };
	uint8_t xPosCounter{ 0 };

	bool objFetchRequested{ false };
	bool objFetcherActive{ false };

	uint16_t VBLANK_CYCLES{};
	uint16_t HBLANK_CYCLES{};
	PPUMode state{ PPUMode::OAMSearch };
	uint16_t videoCycles{ 0 };
};

struct addrPaletteReg
{
	uint8_t value : 6 { 0x00 };
	bool autoIncrement{ false };

	uint8_t read()
	{
		return (static_cast<uint8_t>(autoIncrement) << 7 | value) | 0x40;
	}

	void write(uint8_t val)
	{
		autoIncrement = getBit(val, 7);
		value = val & 0x3F;
	}
};

struct gbcRegs
{
	uint8_t VBK{ 0xFE };
	addrPaletteReg BCPS{};
	addrPaletteReg OCPS{};
};

struct dmgRegs
{
	uint8_t LCDC{ 0x91 };
	uint8_t STAT{ 0x85 };
	uint8_t SCY{ 0x00 };
	uint8_t SCX{ 0x00 };
	uint8_t LYC{ 0x00 };
	uint8_t BGP{ 0xFC };
	uint8_t OBP0{ 0x00 };
	uint8_t OBP1{ 0x00 };
	uint8_t WY{ 0x00 };
	uint8_t WX{ 0x00 };
};

class PPU
{
public:
	static constexpr uint8_t SCR_WIDTH = 160;
	static constexpr uint8_t SCR_HEIGHT = 144;

	static constexpr uint16_t TILES_WIDTH = 16 * 8;
	static constexpr uint16_t TILES_HEIGHT = 24 * 8;

	static constexpr uint32_t FRAMEBUFFER_SIZE = SCR_WIDTH * SCR_HEIGHT * 3;
	static constexpr uint32_t TILEDATA_FRAMEBUFFER_SIZE = TILES_WIDTH * TILES_HEIGHT * 3;

	static inline std::array<PixelOps::color, 4> ColorPalette {};

	static constexpr std::array<color, 4> GRAY_PALETTE = { color {255, 255, 255}, color {169, 169, 169}, color {84, 84, 84}, color {0, 0, 0} };
	static constexpr std::array<color, 4> CLASSIC_PALETTE = { color {155, 188, 15}, color {139, 172, 15}, color {48, 98, 48}, color {15, 56, 15} };
	static constexpr std::array<color, 4> BGB_GREEN_PALETTE = { color {224, 248, 208}, color {136, 192, 112 }, color {52, 104, 86}, color{8, 24, 32} };

	friend class MMU;

	virtual void execute() = 0;
	virtual void reset() = 0;

	virtual void saveState(std::ofstream& st) = 0;
	virtual void loadState(std::ifstream& st) = 0;

	virtual void refreshDMGScreenColors(const std::array<color, 4>& newColorPalette) = 0;
	virtual void renderTileData(uint8_t* buffer, int vramBank) = 0;

	constexpr const uint8_t* getFrameBuffer() { return framebuffer.data(); }
	void (*drawCallback)(const uint8_t* framebuffer) { nullptr };

	inline PPUMode getMode() { return s.state; }
	inline uint16_t getCycles() { return s.videoCycles; }

	dmgRegs regs{};
	gbcRegs gbcRegs{};

protected:
	std::array<uint8_t, FRAMEBUFFER_SIZE> framebuffer{};

	uint8_t* VRAM{ VRAM_BANK0.data() };

	std::array<uint8_t, 160> OAM{};
	std::array<uint8_t, 8192> VRAM_BANK0{};
	std::array<uint8_t, 8192> VRAM_BANK1{};

	std::array<uint8_t, 4> BGpalette{};
	std::array<uint8_t, 4> OBP0palette{};
	std::array<uint8_t, 4> OBP1palette{};

	std::array<uint8_t, 64> BGpaletteRAM{};
	std::array<uint8_t, 64> OBPpaletteRAM{};

	uint8_t objCount{ 0 };
	std::array<OAMobject, 10> selectedObjects{};

	ppuState s{};
	BGPixelFIFO bgFIFO{};
	ObjPixelFIFO objFIFO{};

	bool canAccessOAM{};
	bool canAccessVRAM{};

	inline void updatePalette(uint8_t val, std::array<uint8_t, 4>& palette)
	{
		for (uint8_t i = 0; i < 4; i++)
			palette[i] = (getBit(val, i * 2 + 1) << 1) | getBit(val, i * 2);
	}

	virtual void disableLCD(PPUMode mode = PPUMode::HBlank) = 0;

	inline void writePaletteRAM(std::array<uint8_t, 64>& ram, addrPaletteReg& addr, uint8_t val)
	{
		ram[addr.value] = val;
		if (addr.autoIncrement) addr.value++;
	}
	inline void setVRAMBank(uint8_t val)
	{
		VRAM = val & 0x1 ? VRAM_BANK1.data() : VRAM_BANK0.data();
		gbcRegs.VBK = 0xFE | val;
	}
};