#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include "Mappers/MBCBase.h"
#include "Mappers/RTCTimer.h"

class GBCore;

class Cartridge
{
public:
	static constexpr uint32_t MIN_ROM_SIZE = 0x8000;
	static constexpr uint32_t MAX_ROM_SIZE = 0x800000;

	static constexpr uint32_t ROM_BANK_SIZE = 0x4000;
	static constexpr uint32_t RAM_BANK_SIZE = 0x2000;

	static constexpr bool romSizeValid(uint32_t size) { return size >= MIN_ROM_SIZE && size <= MAX_ROM_SIZE; }
	static inline bool romSizeValid(std::istream& is)
	{
		is.seekg(0, std::ios::end);
		const uint32_t size = is.tellg();
		return romSizeValid(size);
	}

	explicit Cartridge(GBCore& gbCore);

	inline MBCBase* getMapper() const { return mapper.get(); }
	constexpr bool ROMLoaded() const { return romLoaded; }
	constexpr uint8_t getChecksum() const { return checksum; }

	bool hasRAM { false };
	bool hasBattery { false };
	uint16_t romBanks { 0 };
	uint16_t ramBanks { 0 };

	bool hasTimer { false };
	RTCTimer timer { };

	std::vector<uint8_t> rom { };
	std::vector<uint8_t> ram { };

	bool loadROM(std::istream& is);
	uint8_t calculateHeaderChecksum(std::istream& is) const;

	inline void updateSystem()
	{
		if (!romLoaded)
			return;

		updateSystem(rom[0x143]);
	}
private:
	bool proccessCartridgeHeader(std::istream& is, uint32_t fileSize);
	void updateSystem(uint8_t cgbFlag);

	std::unique_ptr<MBCBase> mapper { nullptr };
	GBCore& gbCore;

	bool romLoaded{ false };
	uint8_t checksum{ 0 };
};