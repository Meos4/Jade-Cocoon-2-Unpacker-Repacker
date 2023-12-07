#include "JC2Tools.hpp"
#include "CDData000.hpp"
#include "Types.hpp"

#include "fmt/format.h"

#include <stdexcept>
#include <vector>
#include <fstream>
#include <limits>

namespace JC2Tools
{
	static constexpr auto
		sectorSize{ 2048u },
		locHeaderSize{ 4u };

	static constexpr auto
		cdData000Filename{ "CDDATA.000" },
		cdDataLocFilename{ "CDDATA.LOC" },
		dataDirectory{ "data" };

	struct CdDataLocFileInfo
	{
		u32 position;
		u32 size;
		u32 nbSectors;
		s32 isABin;
	};

	void unpacker(const std::filesystem::path& src, const std::filesystem::path& dest)
	{
		const std::filesystem::path cdData000Path{ fmt::format("{}/{}", src.string(), cdData000Filename) };

		if (!std::filesystem::is_regular_file(cdData000Path))
		{
			throw std::runtime_error{ fmt::format("Can't find \"{}\" in \"{}\"", cdData000Filename, src.string()) };
		}

		const std::filesystem::path cdDataLocPath{ fmt::format("{}/{}", src.string(), cdDataLocFilename) };

		if (!std::filesystem::is_regular_file(cdDataLocPath))
		{
			throw std::runtime_error{ fmt::format("Can't find \"{}\" in \"{}\"", cdDataLocFilename, src.string()) };
		}

		std::ifstream
			cdData000{ cdData000Path, std::ifstream::binary },
			cdDataLoc{ cdDataLocPath, std::ifstream::binary };

		u32 nbFiles;
		cdDataLoc.read((char*)&nbFiles, sizeof(nbFiles));
		const auto locFileInfoSize{ nbFiles * sizeof(CdDataLocFileInfo) };

		if (std::filesystem::file_size(cdDataLocPath) != locFileInfoSize + locHeaderSize)
		{
			throw std::runtime_error{ fmt::format("\"{}\" is invalid", cdDataLocFilename) };
		}

		std::vector<CdDataLocFileInfo> filesInfo(nbFiles);
		cdDataLoc.read((char*)filesInfo.data(), locFileInfoSize);

		const auto maxFileSizeElem{ std::max_element(filesInfo.begin(), filesInfo.end(),
			[](const CdDataLocFileInfo& a, const CdDataLocFileInfo& b)
			{
				return a.size < b.size;
			})};

		std::filesystem::create_directories(dest);

		fmt::print("Unpacking files...\n");

		const auto cdData000FilesPath{ CDData000::filesPath(nbFiles) };
		std::vector<char> buffer(maxFileSizeElem->size);
		auto* const bufferPtr{ buffer.data() };

		for (u32 i{}; i < nbFiles; ++i)
		{
			const std::filesystem::path filePath{ fmt::format("{}/{}", dest.string(), cdData000FilesPath[i]) };
			const auto fileSize{ filesInfo[i].size };
			std::filesystem::create_directories(filePath.parent_path());

			cdData000.seekg(filesInfo[i].position * sectorSize);
			cdData000.read(bufferPtr, fileSize);

			std::ofstream file{ filePath, std::ofstream::binary };
			file.write(bufferPtr, fileSize);
		}

		fmt::print("{} Files unpacked\n", cdData000FilesPath.size());
	}

	void repacker(const std::filesystem::path& src, const std::filesystem::path& dest)
	{
		const std::filesystem::path dataPath{ fmt::format("{}/{}", src.string(), dataDirectory) };

		if (!std::filesystem::is_directory(dataPath))
		{
			throw std::runtime_error{ fmt::format("Can't find \"{}\" directory in \"{}\"", dataDirectory, src.string()) };
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
		u32 maxFileSize{};

		for (const auto filePathStr : cdData000FilesPath)
		{
			const std::filesystem::path filePath{ fmt::format("{}/{}", src.string(), filePathStr) };

			if (!std::filesystem::is_regular_file(filePath))
			{
				throw std::runtime_error{ fmt::format("\"{}\" file is missing in \"{}\"", filePathStr, src.string()) };
			}

			const auto fileSize{ std::filesystem::file_size(filePath) };
			totalFilesSize += fileSize;

			if (fileSize > maxFileSize)
			{
				maxFileSize = static_cast<u32>(fileSize);
			}
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
		const auto remainderBuffer{ maxFileSize % sectorSize };
		std::vector<char> buffer(remainderBuffer ? maxFileSize + sectorSize - remainderBuffer : maxFileSize);
		auto* const bufferPtr{ buffer.data() };
		const std::filesystem::path binExtension{ ".bin" };

		for (const auto filePathStr : cdData000FilesPath)
		{
			const std::filesystem::path filePath{ fmt::format("{}/{}", src.string(), filePathStr) };
			const auto fileSize{ static_cast<u32>(std::filesystem::file_size(filePath)) };
			const CdDataLocFileInfo fileInfo
			{
				.position = sectorPosition,
				.size = fileSize,
				.nbSectors = (fileSize + sectorSize - 1) >> 0xB,
				.isABin = static_cast<s32>(filePath.extension() == binExtension)
			};

			const auto remainder{ fileSize % sectorSize };
			auto fileSizeSector{ fileSize };
			if (remainder)
			{
				const auto padding{ sectorSize - remainder };
				fileSizeSector += padding;
				std::memset(bufferPtr + fileSize, 0, padding);
			}

			std::ifstream file{ filePath, std::ifstream::binary };
			file.read(bufferPtr, fileSize);
			cdData000.write(bufferPtr, fileSizeSector);
			cdDataLoc.write((char*)&fileInfo, sizeof(fileInfo));

			sectorPosition += fileSizeSector / sectorSize;
		}

		fmt::print("Done\n");
	}
}