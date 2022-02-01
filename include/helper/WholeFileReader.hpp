#pragma once

#include <fstream>
#include <limits>
#include <malloc.h>

void* readFile(const char* name, size_t* fileSize) { 
	auto stream = std::ifstream(name, std::ios_base::binary);
	if (!stream.is_open()) {
		*fileSize = 0;
		return nullptr;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	*fileSize = stream.gcount();
	stream.clear();
	stream.seekg(0, std::ios_base::beg);

	void* data = malloc(*fileSize);
	stream.read(reinterpret_cast<char*>(data), *fileSize);
	return data;
}