#pragma once
#include <sstream>
#include <string>
#include <vector>
#include <json/json.h>

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

inline uint32_t asUIntOr(const Json::Value& value, const std::string_view& name, uint32_t fallback) {
	if (value[name.data()].isUInt()) {
		return value[name.data()].asUInt();
	} else {
		return fallback;
	}
}

inline bool asBoolOr(const Json::Value& value, const std::string_view& name, bool fallback) {
	if (value[name.data()].isBool()) {
		return value[name.data()].asBool();
	} else {
		return fallback;
	}
}

inline const char* asCStringOr(const Json::Value& value, const std::string_view& name, const char* fallback) {
	if (value.type() == Json::objectValue && value[name.data()].isString()) {
		return value[name.data()].asCString();
	} else {
		return fallback;
	}
}

inline std::string asStringOr(const Json::Value& value, const std::string_view& name, const std::string& fallback) {
	if (value[name.data()].isString()) {
		return value[name.data()].asString();
	} else {
		return fallback;
	}
}

inline float asFloatOr(const Json::Value& value, const std::string_view& name, float fallback) {
	if (value[name.data()].isNumeric()) {
		return value[name.data()].asFloat();
	} else {
		return fallback;
	}
}