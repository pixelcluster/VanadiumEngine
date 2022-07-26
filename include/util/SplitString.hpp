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
#include <sstream>
#include <string>
#include <vector>

inline std::vector<std::string> splitString(const std::string& list, char separator = '|',
													   bool eraseSpaces = true) {
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
