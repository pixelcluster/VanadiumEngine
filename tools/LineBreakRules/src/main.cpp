#include <cmath>
#include <fstream>
#include <util/UTF8.hpp>
#include <iostream>
#include <vector>

using namespace std::string_literals;

struct CodepointRange {
	uint32_t start;
	uint32_t end;
	std::string breakClass;
};

struct EastAsianWidthCodepointRange {
	uint32_t start;
	uint32_t end;
	std::string eastAsianWidth;
};

struct ExtendedPictographicCodepointRange {
	uint32_t start;
	uint32_t end;
};

uint32_t hexToUint(const std::string& hex) {
	uint32_t result = 0;
	for (size_t i = 0; i < hex.size(); ++i) {
		if (hex[i] >= 'A' && hex[i] <= 'F') {
			result += (static_cast<uint32_t>(hex[i]) - static_cast<uint32_t>('A') + 10) * pow(16, hex.size() - i - 1);
		} else if (hex[i] >= '0' && hex[i] <= '9') {
			result += (static_cast<uint32_t>(hex[i]) - static_cast<uint32_t>('0')) * pow(16, hex.size() - i - 1);
		} else
			return 0;
	}
	return result;
}

std::string stringToClass(const std::string& classString) {
	if (classString == "BK") {
		return "BK";
	} else if (classString == "CM") {
		return "CM";
	} else if (classString == "CR") {
		return "CR";
	} else if (classString == "GL") {
		return "GL";
	} else if (classString == "LF") {
		return "LF";
	} else if (classString == "NL") {
		return "NL";
	} else if (classString == "SP") {
		return "SP";
	} else if (classString == "WJ") {
		return "WJ";
	} else if (classString == "ZW") {
		return "ZW";
	} else if (classString == "ZWJ") {
		return "ZWJ";
	} else if (classString == "AI") {
		return "AI";
	} else if (classString == "AL") {
		return "AL";
	} else if (classString == "B2") {
		return "B2";
	} else if (classString == "BA") {
		return "BA";
	} else if (classString == "BB") {
		return "BB";
	} else if (classString == "CB") {
		return "CB";
	} else if (classString == "CL") {
		return "CL";
	} else if (classString == "CP") {
		return "CP";
	} else if (classString == "EB") {
		return "EB";
	} else if (classString == "EM") {
		return "EM";
	} else if (classString == "EX") {
		return "EX";
	} else if (classString == "H2") {
		return "H2";
	} else if (classString == "H3") {
		return "H3";
	} else if (classString == "HL") {
		return "HL";
	} else if (classString == "HY") {
		return "HY";
	} else if (classString == "ID") {
		return "ID";
	} else if (classString == "IN") {
		return "IN";
	} else if (classString == "IS") {
		return "IS";
	} else if (classString == "JL") {
		return "JL";
	} else if (classString == "JT") {
		return "JT";
	} else if (classString == "JV") {
		return "JV";
	} else if (classString == "NS") {
		return "NS";
	} else if (classString == "NU") {
		return "NU";
	} else if (classString == "OP") {
		return "OP";
	} else if (classString == "PO") {
		return "PO";
	} else if (classString == "PR") {
		return "PR";
	} else if (classString == "QU") {
		return "QU";
	} else if (classString == "RI") {
		return "RI";
	} else if (classString == "SY") {
		return "SY";
	} else if (classString == "AI" || classString == "SG" || classString == "XX" || classString == "SA") {
		return "AL";
	} else if (classString == "CJ") {
		return "NS";
	} else
		return "AL";
}

std::string stringToEastAsianWidth(const std::string_view& string) {
	if (string == "A") {
		return "Other";
	} else if (string == "F") {
		return "FullWidth";
	} else if (string == "H") {
		return "HalfWidth";
	} else if (string == "N") {
		return "Other";
	} else if (string == "Na") {
		return "Other";
	} else if (string == "W") {
		return "Wide";
	}
	return "Other";
}

unsigned int indentationLevel = 0;

void writeLine(std::ofstream& outStream, const std::string& text) {
	for (unsigned int i = 0; i < indentationLevel; ++i) {
		outStream << "\t";
	}
	outStream << text << "\n";
}

int main() {
	std::ifstream inFile = std::ifstream("./UnicodeCharacterAssoc.txt");
	if (!inFile.is_open()) {
		std::cerr << "Could not open character association file!\n";
		return 1;
	}

	std::vector<CodepointRange> breakClassRanges;
	std::vector<EastAsianWidthCodepointRange> eastAsianRanges;
	std::vector<ExtendedPictographicCodepointRange> extendedPictographicRanges;

	std::string currentLine;
	while (std::getline(inFile, currentLine).good()) {
		std::string codepoint;
		std::string category;
		std::string* currentTargetString = &codepoint;

		for (auto item : currentLine) {
			if (item == ' ')
				continue;
			else if (item == '#')
				break;
			else if (item == ';') {
				currentTargetString = &category;
			} else {
				currentTargetString->push_back(item);
			}
		}

		if (codepoint.empty() || category.empty())
			continue;

		std::string firstCodepointString;
		std::string secondCodepointString;
		currentTargetString = &firstCodepointString;

		for (auto item : codepoint) {
			if (item == '.')
				currentTargetString = &secondCodepointString;
			else {
				currentTargetString->push_back(item);
			}
		}

		uint32_t startCodepoint = hexToUint(firstCodepointString);

		breakClassRanges.push_back(
			{ .start = startCodepoint,
			  .end = secondCodepointString.empty() ? startCodepoint : hexToUint(secondCodepointString),
			  .breakClass = stringToClass(category) });
	}

	inFile.close();
	inFile = std::ifstream("./UnicodeEastAsianWidth.txt");
	while (std::getline(inFile, currentLine).good()) {
		std::string codepoint;
		std::string category;
		std::string* currentTargetString = &codepoint;

		for (auto item : currentLine) {
			if (item == ' ')
				continue;
			else if (item == '#')
				break;
			else if (item == ';') {
				currentTargetString = &category;
			} else {
				currentTargetString->push_back(item);
			}
		}

		if (codepoint.empty() || category.empty())
			continue;

		std::string firstCodepointString;
		std::string secondCodepointString;
		currentTargetString = &firstCodepointString;

		for (auto item : codepoint) {
			if (item == '.')
				currentTargetString = &secondCodepointString;
			else {
				currentTargetString->push_back(item);
			}
		}

		uint32_t startCodepoint = hexToUint(firstCodepointString);

		eastAsianRanges.push_back(
			{ .start = startCodepoint,
			  .end = secondCodepointString.empty() ? startCodepoint : hexToUint(secondCodepointString),
			  .eastAsianWidth = stringToEastAsianWidth(category) });
	}

	inFile.close();
	inFile = std::ifstream("./UnicodeExtendedPictographic.txt");
	while (std::getline(inFile, currentLine).good()) {
		std::string codepoint;
		std::string category;
		std::string* currentTargetString = &codepoint;

		for (auto item : currentLine) {
			if (item == ' ')
				continue;
			else if (item == '#')
				break;
			else if (item == ';') {
				currentTargetString = &category;
			} else {
				currentTargetString->push_back(item);
			}
		}

		if (codepoint.empty() || category.empty())
			continue;

		std::string firstCodepointString;
		std::string secondCodepointString;
		currentTargetString = &firstCodepointString;

		for (auto item : codepoint) {
			if (item == '.')
				currentTargetString = &secondCodepointString;
			else {
				currentTargetString->push_back(item);
			}
		}

		uint32_t startCodepoint = hexToUint(firstCodepointString);

		if (category == "Extended_Pictographic") {
			extendedPictographicRanges.push_back(
				{ .start = startCodepoint,
				  .end = secondCodepointString.empty() ? startCodepoint : hexToUint(secondCodepointString) });
		}
	}

	std::ofstream outStream = std::ofstream("./generated_include/CharacterGroup.hpp", std::ios::trunc);

	writeLine(outStream, "#pragma once\n");
	writeLine(outStream, "#include <util/UTF8.hpp>\n");
	writeLine(outStream, "inline BreakClass codepointBreakClass(uint32_t value) {");
	++indentationLevel;
	for (auto& range : breakClassRanges) {
		if (range.start == range.end) {
			writeLine(outStream, "if (value == "s + std::to_string(range.start) + ") {");
		} else {
			writeLine(outStream, "if (value >= "s + std::to_string(range.start) + " && value <= "s +
									 std::to_string(range.end) + ") {");
		}
		++indentationLevel;
		writeLine(outStream, "return BreakClass::"s + range.breakClass + ";");
		--indentationLevel;
		writeLine(outStream, "}");
	}
	// Unspecified ranges, as outlined in the comments of UnicodeCharacterAssoc.txt

	writeLine(outStream, "if ((value >= 0x3400 && value <= 0x4DBF) || (value >= 0x4E00 && value <= 0x9FFF) || "
						 "(value >= 0xF900 && value <= 0xFAFF) || (value >= 0x20000 && value <= 0x2FFFD)"
						 " || (value >= 0x30000 && value <= 0x3FFFD) || (value >= 0x1F000 && value <= 0x1FAFF)"
						 " || (value >= 0x1FC00 && value <= 0x1FFFD)) {");
	++indentationLevel;
	writeLine(outStream, "return BreakClass::ID;");
	--indentationLevel;
	writeLine(outStream, "}");
	writeLine(outStream, "if (value >= 0x20A0 && value <= 0x20CF) {");
	++indentationLevel;
	writeLine(outStream, "return BreakClass::PR;");
	--indentationLevel;
	writeLine(outStream, "}");
	writeLine(outStream, "return BreakClass::AL;");
	--indentationLevel;
	writeLine(outStream, "}\n");

	writeLine(outStream, "inline EastAsianWidth codepointEastAsianWidth(uint32_t value) {");
	++indentationLevel;
	for (auto& range : eastAsianRanges) {
		if (range.eastAsianWidth != "Other") {
			if (range.start == range.end) {
				writeLine(outStream, "if (value == "s + std::to_string(range.start) + ") {");
			} else {
				writeLine(outStream, "if (value >= "s + std::to_string(range.start) + " && value <= "s +
										 std::to_string(range.end) + ") {");
			}
			++indentationLevel;
			writeLine(outStream, "return EastAsianWidth::"s + range.eastAsianWidth + ";");
			--indentationLevel;
			writeLine(outStream, "}");
		}
	}
	writeLine(outStream, "return EastAsianWidth::Other;");
	--indentationLevel;
	writeLine(outStream, "}\n");

	writeLine(outStream, "inline bool isCodepointExtendedPictographic(uint32_t value) {");
	++indentationLevel;
	for (auto& range : eastAsianRanges) {
		if (range.eastAsianWidth != "Other") {
			if (range.start == range.end) {
				writeLine(outStream, "if (value == "s + std::to_string(range.start) + ") {");
			} else {
				writeLine(outStream, "if (value >= "s + std::to_string(range.start) + " && value <= "s +
										 std::to_string(range.end) + ") {");
			}
			++indentationLevel;
			writeLine(outStream, "return true;");
			--indentationLevel;
			writeLine(outStream, "}");
		}
	}
	writeLine(outStream, "return false;");
	--indentationLevel;
	writeLine(outStream, "}");
	outStream.close();
}