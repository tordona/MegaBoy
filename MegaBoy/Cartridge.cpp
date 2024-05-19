#include <iostream>
#include <algorithm>

#include "Cartridge.h"
#include "GBCore.h"
#include "RomOnlyMBC.h"
#include "EmptyMBC.h"
#include "MBC1.h"
#include "MBC2.h"
#include "MBC3.h"
#include "MBC5.h"
#include "HuC1.h"

Cartridge::Cartridge(GBCore& gbCore) : gbCore(gbCore), mapper(std::make_unique<EmptyMBC>(*this)) { }

bool Cartridge::loadROM(std::ifstream& ifs)
{
	ifs.seekg(0, std::ios::end);
	std::ifstream::pos_type size = ifs.tellg();
	if (size < 0x4000) return false; // If file size is less than 1 ROM bank then it is defintely invalid.

	std::vector<uint8_t> fileBuffer;
	fileBuffer.resize(size);

	ifs.seekg(0, std::ios::beg);
	ifs.read(reinterpret_cast<char*>(&fileBuffer[0]), size);

	if (!proccessCartridgeHeader(fileBuffer))
		return false;

	ROMLoaded = true;
	gbCore.reset();

	rom = std::move(fileBuffer);
	calculateChecksum();

	return true;
}

struct importGuard
{
	Cartridge& cartridge;
	uint16_t backupRomBanks;
	uint16_t backupRamBanks;

	importGuard(Cartridge& cartridge) : cartridge(cartridge), backupRomBanks(cartridge.romBanks), backupRamBanks(cartridge.ramBanks)
	{ }

	~importGuard()
	{
		if (!cartridge.readSuccessfully)
		{
			cartridge.romBanks = backupRomBanks;
			cartridge.ramBanks = backupRamBanks;
		}
	}
};

bool Cartridge::proccessCartridgeHeader(const std::vector<uint8_t>& buffer)
{
	readSuccessfully = false;
	importGuard guard{ *this };

	romBanks = 1 << (buffer[0x148] + 1);
	if (romBanks == 0 || romBanks > buffer.size() / 0x4000) return false;

	ramBanks = 0;

	switch (buffer[0x149])
	{
	case 0x02:
		ramBanks = 1;
		break;
	case 0x03:
		ramBanks = 4;
		break;
	case 0x04:
		ramBanks = 16;
		break;
	case 0x05:
		ramBanks = 8;
		break;
	}

	hasBattery = false;
	hasRAM = false; 

	switch (buffer[0x147]) // MBC type
	{
	case 0x00:
		mapper = std::make_unique<RomOnlyMBC>(*this);
		break;
	case 0x01:
		mapper = std::make_unique<MBC1>(*this);
		break;
	case 0x02:
		mapper = std::make_unique<MBC1>(*this);
		break;
	case 0x03:
		hasBattery = true;
		mapper = std::make_unique<MBC1>(*this);
		break;
	case 0x05:
		mapper = std::make_unique<MBC2>(*this);
		break;
	case 0x06:
		hasBattery = true;
		mapper = std::make_unique<MBC2>(*this);
		break;
	case 0x0F:
		hasBattery = true;
		mapper = std::make_unique<MBC3>(*this, true);
		break;
	case 0x10:
		hasBattery = true;
		mapper = std::make_unique<MBC3>(*this, true);
		break;
	case 0x11:
		mapper = std::make_unique<MBC3>(*this, false);
		break;
	case 0x12:
		mapper = std::make_unique<MBC3>(*this, false);
		break;
	case 0x13:
		hasBattery = true;
		mapper = std::make_unique<MBC3>(*this, false);
		break;
	case 0x19:
		mapper = std::make_unique<MBC5>(*this, false);
		break;
	case 0x1A:
		mapper = std::make_unique<MBC5>(*this, false);
		break;
	case 0x1B:
		hasBattery = true;
		mapper = std::make_unique<MBC5>(*this, false);
		break;
	case 0x1C:
		mapper = std::make_unique<MBC5>(*this, true);
		break;
	case 0x1D:
		mapper = std::make_unique<MBC5>(*this, true);
		break;
	case 0x1E:
		hasBattery = true;
		mapper = std::make_unique<MBC5>(*this, true);
		break;
	case 0xFF:
		hasBattery = true;
		mapper = std::make_unique<HuC1>(*this);
		break;

	default:
		std::cout << "Unknown MBC! \n";
		return false;
	}

	gbCore.gameTitle = "";

	for (int i = 0x134; i <= 0x143; i++)
	{
		if (buffer[i] == 0x00) break;
		gbCore.gameTitle += buffer[i];
	}

	readSuccessfully = true;
	return true;
}