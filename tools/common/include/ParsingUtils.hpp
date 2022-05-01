#pragma once
#include <cstdint>
#include <helper/SplitString.hpp>
#include <json/json.h>

inline void* offsetVoidPtr(void* old, size_t byteCount) {
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(old) + byteCount);
}

inline int32_t asIntOr(const Json::Value& value, const std::string_view& name, int32_t fallback) {
	if (value[name.data()].isUInt()) {
		return value[name.data()].asInt();
	} else {
		return fallback;
	}
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