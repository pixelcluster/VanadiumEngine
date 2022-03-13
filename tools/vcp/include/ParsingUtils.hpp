#pragma once
#include <sstream>
#include <string>
#include <vector>

inline std::vector<std::string> splitString(const std::string& list, char separator = '|', bool eraseSpaces = true) {
	std::stringstream listStream = std::stringstream(list);
	std::string listEntry;
	std::vector<std::string> splitList;

	while (std::getline(listStream, listEntry, '|')) {
		if (eraseSpaces) {
			for (size_t i = 0; i < listEntry.size(); ++i) {
				if (listEntry[i] == ' ') {
					listEntry.erase(listEntry.begin() + i);
					--i;
				}
			}
		}
		splitList.push_back(listEntry);
	}
	return splitList;
}

inline void* offsetVoidPtr(void* old, size_t byteCount) {
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(old) + byteCount);
}

enum class PipelineType { Graphics, Compute };