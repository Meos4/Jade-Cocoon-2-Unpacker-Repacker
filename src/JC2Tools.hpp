#pragma once

#include <filesystem>

namespace JC2Tools
{
	void unpacker(const std::filesystem::path& src, const std::filesystem::path& dest);
	void repacker(const std::filesystem::path& src, const std::filesystem::path& dest);
}