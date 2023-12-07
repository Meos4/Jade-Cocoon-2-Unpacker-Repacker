#include "JC2Tools.hpp"

#include "fmt/format.h"

#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <limits>
#include <cstring>

int main(int argc, char** argv)
{
	try
	{
		fmt::print("Jade Cocoon 2 Unpacker / Repacker v1.2.0 by Meos\n\n");

		if (argc < 2)
		{
			fmt::print("0. Unpacker\n1. Repacker\n\n");

			char ch;
			std::cin >> ch;

			fmt::print("\n");

			const auto currentPath{ std::filesystem::current_path() };

			if (ch == '0')
			{
				JC2Tools::unpacker(currentPath, currentPath);
			}
			else if (ch == '1')
			{
				JC2Tools::repacker(currentPath, "Repacked");
			}
		}
		else
		{
			if (std::strcmp(argv[1], "0") == 0 && argc > 3)
			{
				JC2Tools::unpacker(argv[2], argv[3]);
			}
			else if (std::strcmp(argv[1], "1") == 0 && argc > 3)
			{
				JC2Tools::repacker(argv[2], argv[3]);
			}
			else
			{
				throw std::runtime_error
				{
					"Invalid arguments\n"
					"Unpacker arguments: [0] [CDDATA.000 and CDDATA.LOC path] [Unpacked files path]\n"
					"Repacker arguments: [1] [Unpacked files path] [CDDATA.000 and CDDATA.LOC path]\n"
				};
			}
		}
	}
	catch (const std::exception& e)
	{
		fmt::print("Error: {}", e.what());
	}
	
	if (argc < 2)
	{
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cin.get();
	}
}