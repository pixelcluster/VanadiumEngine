/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <vector>
#include <string>
#include <ostream>
#include <sstream>

//defined in generate.cpp
extern unsigned int indentationLevel;

inline std::vector<std::string> splitList(const std::string& list, char separator = ' ') {
	std::stringstream listStream = std::stringstream(list);
	std::string listEntry;
	std::vector<std::string> splitList;

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
	stream << string << "
";
}
