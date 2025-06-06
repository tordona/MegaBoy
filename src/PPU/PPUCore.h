#pragma once
#include <array>
#include <vector>

#include "PPU.h"
#include "../MMU.h"
#include "../CPU/CPU.h"
#include "../appConfig.h"

template <GBSystem sys>
class PPUCore final : public PPU
{
public:
	PPUCore(MMU& mmu, CPU& cpu) : mmu(mmu), cpu(cpu) { }

	void execute() override;
	void reset(bool clearBuf) override;

	void saveState(std::ostream& st) const override;
	void loadState(std::istream& st) override;

	void refreshDMGScreenColors(const std::array<color, 4>& newColors) override;

	void renderTileMap(uint8_t* buffer, uint16_t addr) override;
	void renderTileData(uint8_t* buffer, int vramBank) override;
private:
	MMU& mmu;
	CPU& cpu;

	static constexpr uint16_t TOTAL_SCANLINE_CYCLES = 456;
	static constexpr uint16_t OAM_SCAN_CYCLES = 20 * 4;
	static constexpr uint16_t DEFAULT_VBLANK_LINE_CYCLES = 114 * 4;
	static constexpr uint16_t TOTAL_VBLANK_CYCLES = DEFAULT_VBLANK_LINE_CYCLES * 10;

	inline void invokeDrawCallback(bool firstFrame = false) 
	{
		std::swap(framebuffer, backbuffer);

		if (drawCallback != nullptr)
			drawCallback(framebuffer.get(), firstFrame);
	}

	inline void clearBuffer(bool firstFrame = false)
	{
		PixelOps::clearBuffer(backbuffer.get(), SCR_WIDTH, SCR_HEIGHT, sys == GBSystem::DMG ? PPU::ColorPalette[0] : color { 255, 255, 255 });
		invokeDrawCallback(firstFrame);
	}

	void updateInterrupts();
	void SetPPUMode(PPUMode ppuState);
	void setLCDEnable(bool val) override;

	bool canReadVRAM() override;
	bool canReadOAM() override;
	bool canWriteVRAM() override;
	bool canWriteOAM() override;

	void handleOAMSearch();
	void handleHBlank();
	void handleVBlank();
	void handlePixelTransfer();

	void resetPixelTransferState();

	void tryStartSpriteFetcher();
	void executeBGFetcher();
	void executeObjFetcher();
	void renderFIFOs();

	constexpr color getPixel(uint8_t x, uint8_t y) const { return PixelOps::getPixel(framebuffer.get(), SCR_WIDTH, x, y); }
	constexpr void setPixel(uint8_t x, uint8_t y, color c) { PixelOps::setPixel(backbuffer.get(), SCR_WIDTH, x, y, c); }

	constexpr uint8_t getColorID(uint8_t tileLow, uint8_t tileHigh, uint8_t ind) const
	{
		return ((tileHigh >> ind) & 1) << 1 | ((tileLow >> ind) & 1);
	}

	template <bool obj, bool mainTexture = false>
	constexpr color getColor(uint8_t colorID, uint8_t palette)
	{
		if constexpr (System::IsCGBDevice(sys))
		{
			const uint8_t* paletteRamPtr;

			if constexpr (obj)
				paletteRamPtr = gbcRegs.OCPS.RAM.data();
			else
				paletteRamPtr = gbcRegs.BCPS.RAM.data();

			if constexpr (sys == GBSystem::DMGCompatMode)
			{
				if constexpr (obj)
					colorID = palette == 0 ? OBP0[colorID] : OBP1[colorID];
				else
					colorID = BGP[colorID];
			}

			const int paletteRAMInd { palette * 8 + colorID * 2 };
			const uint16_t rgb5 = paletteRamPtr[paletteRAMInd + 1] << 8 | paletteRamPtr[paletteRAMInd];

			// If rendering main texutre, don't use color correction, let frontend deal with it (doing it in opengl shader instead).
			if constexpr (mainTexture)
				return color::fromRGB5(rgb5, false);
			else
				return color::fromRGB5(rgb5, appConfig::gbcColorCorrection);
		}
		else
		{
			uint8_t* palettePtr;

			if constexpr (obj)
				palettePtr = palette == 0 ? OBP0.data() : OBP1.data();
			else
				palettePtr = BGP.data();

			return PPU::ColorPalette[palettePtr[colorID]];
		}
	}

	inline uint16_t getBGTileAddr(uint8_t tileInd) const
	{
		return BGUnsignedAddressing() ? tileInd * 16 : 0x1000 + static_cast<int8_t>(tileInd) * 16;
	}

	inline uint8_t getBGTileOffset() const
	{
		const uint8_t bgLineOffset = (s.LY + s.SCYlatch) % 8;
		const uint8_t windowLineOffset = s.WLY % 8;

		if constexpr (sys == GBSystem::DMG)
			return 2 * (bgFIFO.s.fetchingWindow ? windowLineOffset : bgLineOffset);
		else
		{
			const bool yFlip = getBit(bgFIFO.s.cgbAttributes, 6);
			return 2 * (bgFIFO.s.fetchingWindow ? (yFlip ? 7 - windowLineOffset : windowLineOffset) : (yFlip ? 7 - bgLineOffset : bgLineOffset));
		}
	}
	inline uint8_t getObjTileOffset(const OAMobject& obj) const
	{
		const bool yFlip = getBit(obj.attributes, 6);
		return static_cast<uint8_t>(2 * (yFlip ? (obj.Y - s.LY + 7) : (8 - (obj.Y - s.LY + 8))));
	}

	inline bool GBCMasterPriority() const { return !getBit(regs.LCDC, 0); }
	inline bool DMGTileMapsEnable() const { return getBit(regs.LCDC, 0); }

	inline bool OBJEnable() const { return getBit(regs.LCDC, 1); }
	inline bool DoubleOBJSize() const { return getBit(regs.LCDC, 2); }
	inline uint16_t BGTileMapAddr() const { return getBit(regs.LCDC, 3) ? 0x1C00 : 0x1800; }
	inline uint16_t WindowTileMapAddr() const { return getBit(regs.LCDC, 6) ? 0x1C00 : 0x1800; }
	inline bool BGUnsignedAddressing() const { return getBit(regs.LCDC, 4); }
	inline bool WindowEnable() const { return getBit(regs.LCDC, 5); }
	inline bool LCDEnabled() const { return getBit(regs.LCDC, 7); }

	inline bool LYC_STAT() const { return getBit(regs.STAT, 6); }
	inline bool OAM_STAT() const { return getBit(regs.STAT, 5); }
	inline bool VBlank_STAT() const { return getBit(regs.STAT, 4); }
	inline bool HBlank_STAT() const { return getBit(regs.STAT, 3); }
};