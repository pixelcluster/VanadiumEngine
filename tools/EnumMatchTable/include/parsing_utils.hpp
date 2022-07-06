#pragma once
#include <util/Vector.hpp>
#include <string>
#include <ostream>
#include <sstream>

//defined in generate.cpp
extern unsigned int indentationLevel;

inline vanadium::SimpleVector<std::string> splitList(const std::string& list, char separator = ' ') {
	std::stringstream listStream = std::stringstream(list);
	std::string listEntry;
	vanadium::SimpleVector<std::string> splitList;

	while (std::getline(listStream, listEntry, separator))
	{
		splitList.push_back(listEntry);
	}
	return splitList;
}

inline void addLine(std::ostream& stream, const std::string& string) {
	for (unsigned int i = 0; i < indentationLevel; ++i) {
		stream << "	";
	}
	stream << string << "\n";
}