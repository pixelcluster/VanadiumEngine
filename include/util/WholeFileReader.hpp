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

#include <fstream>
#include <limits>
#include <malloc.h>

//Read a whole file.
//Returned data must be casted to char* and delete[]'d.
inline void* readFile(const char* name, size_t* fileSize) { 
	auto stream = std::ifstream(name, std::ios_base::binary);
	if (!stream.is_open()) {
		*fileSize = 0;
		return nullptr;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	*fileSize = stream.gcount();
	stream.clear();
	stream.seekg(0, std::ios_base::beg);

	char* data = new char[*fileSize];
	stream.read(reinterpret_cast<char*>(data), *fileSize);
	return data;
}
