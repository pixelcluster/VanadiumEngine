#pragma once
#include <string>
#include <vector>
#include <sstream>

inline std::vector<std::string> splitFlags(const std::string& list) {
	std::stringstream listStream = std::stringstream(list);
	std::string listEntry;
	std::vector<std::string> splitList;

	while (std::getline(listStream, listEntry, '|')) {
		for (size_t i = 0; i < listEntry.size(); ++i) {
			if (listEntry[i] == ' ') {
				listEntry.erase(listEntry.begin() + i);
				--i;
			}
		}
		splitList.push_back(listEntry);
	}
	return splitList;
}