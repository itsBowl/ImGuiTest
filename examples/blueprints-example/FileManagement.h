#pragma once
#include <vector>
#include <filesystem>
#include <string>
#include <utility>
#include <fstream>

namespace FileManagment
{
	bool getFilesInFolder(std::vector<std::string>* loc, std::string folder)
	{
		if (loc == nullptr || folder == "") return false;
		std::filesystem::directory_iterator fsIt = std::filesystem::directory_iterator(folder);

		for (const auto& entry : std::filesystem::directory_iterator(fsIt))
		{
			auto path = entry.path().string();
			loc->push_back(path);
		}
		return true;
	}

	bool getFilesInFolder(std::vector<std::pair<std::string, std::string>>* loc, std::string folder)
	{
		if (loc == nullptr || folder == "") return false;
		std::filesystem::directory_iterator fsIt = std::filesystem::directory_iterator(folder);

		for (const auto& entry : std::filesystem::directory_iterator(fsIt))
		{
			auto path = entry.path().string();
			auto extention = entry.path().extension().string();
			loc->push_back(std::make_pair(path, extention));
		}
		return true;
	}

	bool readFile(const std::string& fileName, std::vector<char>* dst)
	{
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);
		if (!file.is_open()) return false;
		size_t fileSize = (size_t)file.tellg();
		if (fileSize == 0) return false;
		std::vector<char> buffer(fileSize + 1);

		file.seekg(std::ios::beg);
		file.read(dst->data(), fileSize);
		file.close();

		return true;
	}
}

