#pragma once

#include <array>
#include <util/UTF8.hpp>
#include <robin_hood.h>
#include <string>
#include <string_view>
#include <utility>
#include <util/Vector.hpp>

namespace vanadium::ui {

	// https://www.unicode.org/reports/tr14/
	// Identifiers that are different to the ones used in the annex:
	//_ discards East Asian Full width, wide and Half width characters
	//< forces a token to be at the start of the line
	// p identifies characters with the Extended_Pictographic property

	// https://www.unicode.org/reports/tr14/#BreakingRules
	constexpr std::array<std::string_view, 64> defaultLinebreakRules = {
		"x CM",		// LB10, see Section 9.1
		"BK !",	   // LB4,
		"CR x LF", // LB5
		"CR !",
		"LF !",
		"NL !",
		"x (BK | CR | LF | NL)", // LB6
		"x SP",					 // LB7
		"x ZW",
		"ZW SP* %", // LB8
		"ZWJ x",	// LB8a
		"x WJ CM*", // LB11
		"WJ CM* x",
		"GL CM* x",				// LB12
		"(SP BA HY)^ CM* x GL", // LB12a
		"NU^ CM* x CL",			// LB13, see Section 8.2 Example 7
		"NU^ CM* x CP",
		"NU^ CM* x EX",
		"NU^ CM* x IS",
		"NU^ CM* x SY",
		"OP CM* SP* x", // LB14
		"QU CM* SP* x OP",
		"(CL | CP) CM* SP* x NS", // LB16
		"B2 CM* SP* x B2",		  // LB17
		"SP %",					  // LB18
		"x QU",					  // LB19
		"QU CM* x",
		"% CB", // LB20
		"CB CM* %",
		"x BA", // LB21
		"x HY",
		"x NS",
		"BB CM* x",
		"HL CM* (HY | BA) CM* x", // LB21a
		"SY CM* x HL",			  // LB21b
		"x IN",					  // LB22
		"(AL | HL) CM* x NU",	  // LB23
		"CM CM* x NU",			  // for LB10, see Section 9.1
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
		"JL CM* x (JL | JV | H2 | H3)", // LB26
		"(JV | H2) CM* x (JV | JT)",
		"(JT | H3) CM* x JT",
		"(JL | JV | JT | H2 | H3) CM* x PO", // LB27
		"PR CM* x (JL | JV | JT | H2 | H3)",
		"CM x (AL | HL)",			 // for LB10, see Section 9.1
		"(AL | HL) CM* x (AL | HL)", // LB28
		"IS CM* x (AL | HL)",		 // LB29
		"(AL | HL | NU) CM* x OP_",	 // LB30
		"CM x OP_",					 // for LB10, see Section 9.1
		"OP_ CM* x (AL | HL | NU)",
		"(RI RI)<* RI CM* x RI", // LB30a
		"(RI)^ (RI RI)* RI CM* x RI",
		"EB CM* x EM", // LB30b
		"EBp CM* x EM",
		"%" // LB31
	};

	enum class LinebreakStatus { Undefined, DoNotBreak, Opportunity, Mandatory };

	struct BreakClassRuleQuantifier {
		uint32_t minCount = 1;
		uint32_t maxCount = 1;
	};

	struct BreakClassRuleModifier {
		bool valid = true;
		bool negated = false;
		bool excludeEastAsianWidths = false;
		bool mustBeAtStart = false;
		bool matchExtendedPictographics = false;
		bool optional = false;
		BreakClassRuleQuantifier quantifier;
	};

	struct BreakClassRuleToken {
		SimpleVector<BreakClass> classOptions;
		BreakClassRuleModifier modifier;
	};

	struct BreakClassRuleAction {
		LinebreakStatus newStatus = LinebreakStatus::Undefined;
		uint32_t beforeIndex;
	};

	struct BreakClassRule {
		bool isValid = false;
		SimpleVector<BreakClassRuleToken> tokens;
		BreakClassRuleAction action;
	};

	struct BreakClassRuleTraits {
		BreakClass breakClass;
		EastAsianWidth eastAsianWidth;
		bool isExtendedPictographic;
	};

	enum class ParserState { Start, Token, ListToken, Error, Max };

	enum class StackAction { Keep, Push, Max };

	struct StateAction {
		ParserState newState = ParserState::Error;
		StackAction action = StackAction::Keep;
	};

	enum class InputSymbolClass {
		Class,
		OpenParenthesis,
		CloseParenthesis,
		VerticalBar,
		Modifier,
		Whitespace,
		BreakAction,
		Max
	};

	struct BreakClassStackEntry {
		InputSymbolClass symbolClass;
		std::string text;
	};

	// For each possible state, an array of actions
	static std::array<robin_hood::unordered_map<InputSymbolClass, StateAction>, static_cast<uint32_t>(ParserState::Max)>
		tokenizerFSM = {
			// Start
			robin_hood::unordered_map<InputSymbolClass, StateAction>{
				{ { InputSymbolClass::Class,
					StateAction{ .newState = ParserState::Token, .action = StackAction::Push } },
				  { InputSymbolClass::OpenParenthesis,
					StateAction{ .newState = ParserState::ListToken, .action = StackAction::Push } },
				  { InputSymbolClass::BreakAction,
					StateAction{ .newState = ParserState::Token, .action = StackAction::Push } } } },
			// Token
			robin_hood::unordered_map<InputSymbolClass, StateAction>{
				{ { InputSymbolClass::Class,
					StateAction{ .newState = ParserState::Token, .action = StackAction::Push } },
				  { InputSymbolClass::OpenParenthesis,
					StateAction{ .newState = ParserState::ListToken, .action = StackAction::Push } },
				  { InputSymbolClass::Whitespace,
					StateAction{ .newState = ParserState::Token, .action = StackAction::Push } },
				  { InputSymbolClass::BreakAction,
					StateAction{ .newState = ParserState::Token, .action = StackAction::Push } },
				  { InputSymbolClass::Modifier,
					StateAction{ .newState = ParserState::Token, .action = StackAction::Push } } } },
			// ListToken
			robin_hood::unordered_map<InputSymbolClass, StateAction>{
				{ { InputSymbolClass::Class,
					StateAction{ .newState = ParserState::ListToken, .action = StackAction::Push } },
				  { InputSymbolClass::CloseParenthesis,
					StateAction{ .newState = ParserState::Token, .action = StackAction::Push } },
				  { InputSymbolClass::Whitespace,
					StateAction{ .newState = ParserState::ListToken, .action = StackAction::Push } },
				  { InputSymbolClass::VerticalBar,
					StateAction{ .newState = ParserState::ListToken, .action = StackAction::Push } } } },
			// Error
			robin_hood::unordered_map<InputSymbolClass, StateAction>{}
		};

	struct Parser {
		SimpleVector<BreakClassStackEntry> stack;
		ParserState currentState;
	};

	void executeRule(const SimpleVector<BreakClassRuleTraits>& classString, SimpleVector<LinebreakStatus>& statusArray,
					 const std::string_view& ruleString);

	bool tryMatchToken(const SimpleVector<BreakClassRuleTraits>& classString, uint32_t& prevMatchCount, uint32_t& index,
					   const BreakClassRuleToken& token);
	bool currentTokenMatches(const BreakClassRuleTraits& traits, uint32_t& index, const BreakClassRuleToken& token);

	BreakClassRule parseRule(const std::string_view& rule);
	InputSymbolClass inputSymbolClass(char inputSymbol);
} // namespace vanadium::ui
