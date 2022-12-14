#include "JC2Tools.hpp"
#include "CDData000.hpp"
#include "Types.hpp"

#include "fmt/format.h"

#include <stdexcept>
#include <vector>
#include <fstream>
#include <limits>

static constexpr auto
	sectorSize{ 2048u },
	locHeaderSize{ 4u };

static constexpr auto
	cdData000Filename{ "CDDATA.000" }, // Pal - Ntsc-j
	cdData000FilenameLow{ "cddata.000" }, // Ntsc-u
	cdDataLocFilename{ "CDDATA.LOC" },
	dataDirectory{ "data" };

struct CdDataLocFileInfo
{
	u32 position;
	u32 size;
	u32 nbSectors;
	u32 unknown;
};

namespace JC2Tools
{
	void unpacker(const std::filesystem::path& src, const std::filesystem::path& dest)
	{
		const std::filesystem::path cdData000Path{ fmt::format("{}/{}", src.string(), cdData000Filename) };

		if (!std::filesystem::is_regular_file(cdData000Path) &&
			!std::filesystem::is_regular_file(fmt::format("{}/{}", src.string(), cdData000FilenameLow)))
		{
			throw std::runtime_error{ fmt::format("Can't find \"{}\" or \"{}\" in \"{}\"", cdData000Filename, cdData000FilenameLow, src.string()) };
		}

		const std::filesystem::path cdDataLocPath{ fmt::format("{}/{}", src.string(), cdDataLocFilename) };

		if (!std::filesystem::is_regular_file(cdDataLocPath))
		{
			throw std::runtime_error{ fmt::format("Can't find \"{}\" in \"{}\"", cdDataLocPath.filename().string(), cdDataLocPath.parent_path().string()) };
		}

		std::ifstream
			cdData000{ cdData000Path, std::ifstream::binary },
			cdDataLoc{ cdDataLocPath, std::ifstream::binary };

		u32 nbFiles;
		cdDataLoc.read((char*)&nbFiles, sizeof(nbFiles));

		if (std::filesystem::file_size(cdDataLocPath) != nbFiles * sizeof(CdDataLocFileInfo) + locHeaderSize)
		{
			throw std::runtime_error{ fmt::format("\"{}\" is invalid", cdDataLocPath.filename().string()) };
		}

		std::filesystem::create_directories(dest);

		fmt::print("Unpacking files...\n");

		const auto cdData000FilesPath{ CDData000::filesPath(nbFiles) };

		for (const auto& filePathStr : cdData000FilesPath)
		{
			const std::filesystem::path filePath{ fmt::format("{}/{}", dest.string(), filePathStr) };
			std::filesystem::create_directories(filePath.parent_path());

			CdDataLocFileInfo fileInfo;
			cdDataLoc.read((char*)&fileInfo, sizeof(fileInfo));

			std::vector<char> buffer(fileInfo.size);
			cdData000.seekg(fileInfo.position * sectorSize);
			cdData000.read(buffer.data(), fileInfo.size);

			std::ofstream file{ filePath, std::ofstream::binary };
			file.write((const char*)buffer.data(), buffer.size());
		}

		fmt::print("{} Files unpacked\n", cdData000FilesPath.size());
	}

	void repacker(const std::filesystem::path& src, const std::filesystem::path& dest)
	{
		const std::filesystem::path dataPath{ fmt::format("{}/{}", src.string(), dataDirectory) };

		if (!std::filesystem::is_directory(dataPath))
		{
			throw std::runtime_error{ fmt::format("Can't find \"{}\" directory in \"{}\"", dataPath.filename().string(), dataPath.parent_path().string()) };
		}

		u32 nbFiles{};

		for (const auto& directoryContent : std::filesystem::recursive_directory_iterator{ dataPath })
		{
			if (std::filesystem::is_regular_file(directoryContent))
			{
				++nbFiles;
			}
		}

		const auto cdData000FilesPath{ CDData000::filesPath(nbFiles) };
		u64 totalFilesSize{};

		for (const auto& filePathStr : cdData000FilesPath)
		{
			const std::filesystem::path filePath{ fmt::format("{}/{}", src.string(), filePathStr) };

			if (!std::filesystem::is_regular_file(filePath))
			{
				throw std::runtime_error{ fmt::format("\"{}\" file is missing in \"{}\"", filePath.filename().string(), filePath.parent_path().string()) };
			}

			totalFilesSize += std::filesystem::file_size(filePath);
		}

		if (totalFilesSize > std::numeric_limits<u32>::max())
		{
			throw std::runtime_error{ fmt::format("\"{}\" can't be repacked because files exceed the size limit", cdData000Filename) };
		}

		std::filesystem::create_directories(dest);

		std::ofstream
			cdData000{ fmt::format("{}/{}", dest.string(), cdData000Filename), std::ofstream::binary },
			cdDataLoc{ fmt::format("{}/{}", dest.string(), cdDataLocFilename), std::ofstream::binary };

		fmt::print("Repacking files...\n");

		cdDataLoc.write((char*)&nbFiles, sizeof(nbFiles));

		u32 sectorPosition{};

		for (const auto& filePathStr : cdData000FilesPath)
		{
			const std::filesystem::path filePath{ fmt::format("{}/{}", src.string(), filePathStr) };
			const auto fileSize{ static_cast<u32>(std::filesystem::file_size(filePath)) };
			const CdDataLocFileInfo fileInfo
			{
				.position = sectorPosition,
				.size = fileSize,
				.nbSectors = (fileSize + sectorSize - 1) >> 0xB,
				.unknown = 0
			};

			const auto rest{ fileSize % sectorSize };
			std::vector<char> buffer(rest ? fileSize + sectorSize - rest : fileSize);
			std::ifstream file{ filePath, std::ifstream::binary };

			file.read(buffer.data(), fileSize);
			cdData000.write(buffer.data(), buffer.size());
			cdDataLoc.write((char*)&fileInfo, sizeof(fileInfo));

			sectorPosition += static_cast<u32>(buffer.size()) / sectorSize;
		}

		fmt::print("Done\n");
	}
}