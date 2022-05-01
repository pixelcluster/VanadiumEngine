#pragma once

#include <array>
#include <helper/UTF8.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace vanadium::ui {

	// https://www.unicode.org/reports/tr14/
	// Identifiers that are different to the ones used in the annex:
	//_ discards East Asian Full width, wide and Half width characters
	//< forces a token to be at the start of the line
	//p identifies characters with the Extended_Pictographic property

	// https://www.unicode.org/reports/tr14/#BreakingRules
	constexpr std::array<std::string_view, 64> defaultLinebreakRules = {
		"BK !",	   // LB4,
		"CR x LF", // LB5
		"CR !",
		"LF !",
		"NL !",
		"x (BK | CR | LF | NL)", // LB6
		"x SP",					   // LB7
		"x ZW",
		"ZW SP* %", // LB8
		"ZWJ x",	// LB8a
		"x CM",		// LB10, see Section 9.1
		"x WJ CM*", // LB11
		"WJ CM* x",
		"GL CM* x", //LB12
		"^(SP BA HY) CM* x GL", //LB12a
		"^NU CM* x CL", // LB13, see Section 8.2 Example 7
		"^NU CM* x CP",
		"^NU CM* x EX",
		"^NU CM* x IS",
		"^NU CM* x SY",
		"OP CM* SP* x", //LB14
		"QU CM* SP* x OP",
		"(CL | CP) CM* SP* x NS", //LB16
		"B2 CM* SP* x B2", //LB17
		"SP %", //LB18
		"x QU", //LB19
		"QU CM* x",
		"% CB", //LB20
		"CB CM* %",
		"x BA", //LB21
		"x HY",
		"x NS",
		"BB CM* x",
		"HL CM* (HY | BA) CM* x", //LB21a
		"SY CM* x HL", //LB21b
		"x IN", //LB22
		"(AL | HL) CM* x NU", //LB23
		"CM CM* x NU", // for LB10, see Section 9.1
		"NU CM* x (AL | HL)",
		"PR CM* x (ID | EB | EM)",
		"(ID | EB | EM) CM* x PO",
		"(PR | PO) CM* x (AL | HL)",
		"(AL | HL) CM* x (PR | PO)",
		"(PR | PO) CM* x ( OP | HY )? NU", // LB25, see Section 8.2 Example 7
		"(OP | HY) CM* x NU",
		"NU CM* x (NU | SY | IS)",
		"NU CM* (NU | SY | IS | CM)* x (NU | SY | IS | CL | CP )",
		"NU CM* (NU | SY | IS | CM)* (CL | CP)? x (PO | PR)",
		"JL CM* x (JL | JV | H2 | H3)", //LB26
		"(JV | H2) CM* x (JV | JT)",
		"(JT | H3) CM* x JT",
		"(JL | JV | JT | H2 | H3) CM* x PO", //LB27
		"PR CM* x (JL | JV | JT | H2 | H3)",
		"CM x (AL | HL)", // for LB10, see Section 9.1
		"(AL | HL) CM* x (AL | HL)", //LB28
		"IS CM* x (AL | HL)", //LB29
		"(AL | HL | NU) CM* x OP_", //LB30
		"CM x OP_", // for LB10, see Section 9.1
		"OP_ CM* x (AL | HL | NU)",
		"<(RI RI)* RI CM* x RI", //LB30a
		"^(RI) (RI RI)* RI CM* x RI",
		"EB CM* x EM", //LB30b
		"EBp CM* x EM",
		"%" //LB31
	};

	enum class LinebreakStatus { Undefined, DoNotBreak, Opportunity, Mandatory };

	struct BreakClassRuleQuantifier {
		uint32_t minCount = 1;
		uint32_t maxCount = 1;
	};

	struct BreakClassRuleModifier
	{
		bool valid = true;
		bool negated = false;
		bool excludeEastAsianWidths = false;
		bool mustBeAtStart = false;
		bool matchExtendedPictographics = false;
		bool allOptionsRequired = true;
		BreakClassRuleQuantifier quantifier;
	};

	struct BreakClassRuleToken {
		std::vector<BreakClass> classOptions;
		BreakClassRuleModifier modifier;
	};

	struct BreakClassRuleAction {
		LinebreakStatus newStatus = LinebreakStatus::Undefined;
		uint32_t beforeIndex;
	};

	struct BreakClassRule {
		bool isValid = false;
		std::vector<BreakClassRuleToken> tokens;
		BreakClassRuleAction action;
	};

	struct BreakClassRuleTraits {
		BreakClass breakClass;
		EastAsianWidth eastAsianWidth;
		bool isExtendedPictographic;
	};

	void executeRule(const std::vector<BreakClassRuleTraits>& classString, std::vector<LinebreakStatus>& statusArray,
					 const std::string_view& ruleString);

	bool tryMatchToken(const std::vector<BreakClassRuleTraits>& classString, uint32_t& prevMatchCount, uint32_t& index, const BreakClassRuleToken& token);
	bool currentTokenMatches(const BreakClassRuleTraits& traits, uint32_t& index, const BreakClassRuleToken& token);

	BreakClassRule parseRule(const std::string_view& rule);

	BreakClassRuleModifier parseModifiers(const std::string_view& rule, size_t& index);
	bool isModifier(char character);
	bool isBreakAction(char character);

	char peekNext(const std::string_view& rule, size_t index);
	void ignoreWhitespace(const std::string_view& rule, size_t& index);
} // namespace vanadium::ui
