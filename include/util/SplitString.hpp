#include <sstream>
#include <string>
#include <util/Vector.hpp>

inline vanadium::SimpleVector<std::string> splitString(const std::string& list, char separator = '|',
													   bool eraseSpaces = true) {
	std::stringstream listStream = std::stringstream(list);
	std::string listEntry;
	vanadium::SimpleVector<std::string> splitList;

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