#pragma once
#include <cstdint>
#include <string_view>

inline uint32_t utf8CharSize(char firstCodepointByte) {
	if (!(firstCodepointByte & 0x80))
		return 1U;
	else if ((firstCodepointByte & 0xC0) == 0xC0)
		return 2U;
	else if ((firstCodepointByte & 0xE0) == 0xE0)
		return 3U;
	else if ((firstCodepointByte & 0xF0) == 0xF0)
		return 4U;
	else
		return 0U;
}

inline uint32_t utf8Codepoint(const char* codeBegin, uint32_t maxSize) {
	if(maxSize == 0) return 0U;

	uint32_t size = utf8CharSize(*codeBegin);
	if(size > maxSize) return 0U;

	uint32_t result = 0;
	for(uint32_t i = 0; i < size; ++i) {
		result |= *codeBegin << i * 8U;
	}
	return result;
}

// https://www.unicode.org/reports/tr14/
enum class BreakClass {
	BK,
	CM,
	CR,
	GL,
	LF,
	NL,
	SP,
	WJ,
	ZW,
	ZWJ,
	AI,
	AL,
	B2,
	BA,
	BB,
	CB,
	CL,
	CP,
	EB,
	EM,
	EX,
	H2,
	H3,
	HL,
	HY,
	ID,
	INSEP,
	IS,
	JL,
	JT,
	JV,
	NS,
	NU,
	OP,
	PO,
	PR,
	QU,
	RI,
	SY
};

enum class EastAsianWidth {
	Other, FullWidth, Wide, HalfWidth
};

inline BreakClass breakClassFromString(const std::string_view& string) {
	if (string == "BK") {
		return BreakClass::BK;
	} else if (string == "CM") {
		return BreakClass::CM;
	} else if (string == "CR") {
		return BreakClass::CR;
	} else if (string == "GL") {
		return BreakClass::GL;
	} else if (string == "LF") {
		return BreakClass::LF;
	} else if (string == "NL") {
		return BreakClass::NL;
	} else if (string == "SP") {
		return BreakClass::SP;
	} else if (string == "WJ") {
		return BreakClass::WJ;
	} else if (string == "ZW") {
		return BreakClass::ZW;
	} else if (string == "ZWJ") {
		return BreakClass::ZWJ;
	} else if (string == "AI") {
		return BreakClass::AI;
	} else if (string == "AL") {
		return BreakClass::AL;
	} else if (string == "B2") {
		return BreakClass::B2;
	} else if (string == "BA") {
		return BreakClass::BA;
	} else if (string == "BB") {
		return BreakClass::BB;
	} else if (string == "CB") {
		return BreakClass::CB;
	} else if (string == "CL") {
		return BreakClass::CL;
	} else if (string == "CP") {
		return BreakClass::CP;
	} else if (string == "EB") {
		return BreakClass::EB;
	} else if (string == "EM") {
		return BreakClass::EM;
	} else if (string == "EX") {
		return BreakClass::EX;
	} else if (string == "H2") {
		return BreakClass::H2;
	} else if (string == "H3") {
		return BreakClass::H3;
	} else if (string == "HL") {
		return BreakClass::HL;
	} else if (string == "HY") {
		return BreakClass::HY;
	} else if (string == "ID") {
		return BreakClass::ID;
	} else if (string == "IN") {
		return BreakClass::INSEP;
	} else if (string == "IS") {
		return BreakClass::IS;
	} else if (string == "JL") {
		return BreakClass::JL;
	} else if (string == "JT") {
		return BreakClass::JT;
	} else if (string == "JV") {
		return BreakClass::JV;
	} else if (string == "NS") {
		return BreakClass::NS;
	} else if (string == "NU") {
		return BreakClass::NU;
	} else if (string == "OP") {
		return BreakClass::OP;
	} else if (string == "PO") {
		return BreakClass::PO;
	} else if (string == "PR") {
		return BreakClass::PR;
	} else if (string == "QU") {
		return BreakClass::QU;
	} else if (string == "RI") {
		return BreakClass::RI;
	} else if (string == "SY") {
		return BreakClass::SY;
	}
	return static_cast<BreakClass>(~0U);
}