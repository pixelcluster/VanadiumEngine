#include <Log.hpp>
#include <ui/util/BreakClassRule.hpp>

namespace vanadium::ui {
	void executeRule(const std::vector<BreakClassRuleTraits>& classString, std::vector<LinebreakStatus>& statusArray,
					 const std::string_view& ruleString) {
		BreakClassRule rule = parseRule(ruleString);
		if (!rule.isValid)
			return;

		std::vector<uint32_t> matchIndices;
		if (rule.tokens.empty()) {
			matchIndices.reserve(classString.size());
			for (unsigned int i = 0; i < classString.size(); ++i) {
				matchIndices.push_back(i);
			}
		} else {
			for (uint32_t i = 0; i < classString.size();) {
				if (currentTokenMatches(classString[i], i, rule.tokens[0])) {
					uint32_t matchesInARow = 1;
					uint32_t j = i + 1;
					uint32_t tokenIndex = 0;
					uint32_t matchIndex = 0;
					bool lastTokenMatched = false;

					while (j < classString.size() && tokenIndex < rule.tokens.size()) {
						if (tokenIndex == rule.action.beforeIndex) {
							matchIndex = j;
						}
						if (tryMatchToken(classString, matchesInARow, j, rule.tokens[tokenIndex])) {
							if (tokenIndex == rule.tokens.size() - 1) {
								lastTokenMatched = true;
							}
						}
						matchesInARow = 0;
						++tokenIndex;
					}
					if (tokenIndex == rule.action.beforeIndex) {
						matchIndex = j;
					}
					if (rule.action.beforeIndex == 0) {
						matchIndex = i;
					}
					if(rule.tokens.size() == 1) {
						lastTokenMatched = true;
					}

					if (lastTokenMatched) {
						matchIndices.push_back(matchIndex);
					}
				}
				++i;
			}
		}

		for (auto index : matchIndices) {
			if (index - 1U < statusArray.size()) {
				if (statusArray[index - 1U] == LinebreakStatus::Undefined) {
					statusArray[index - 1U] = rule.action.newStatus;
				}
			}
		}
		// A linebreak will be inserted unconditionally, prevent accidental additional linebreaks
		statusArray[statusArray.size() - 1] = LinebreakStatus::DoNotBreak;
	}

	bool tryMatchToken(const std::vector<BreakClassRuleTraits>& classString, uint32_t& prevMatchCount, uint32_t& index,
					   const BreakClassRuleToken& token) {
		if (prevMatchCount == token.modifier.quantifier.maxCount) {
			return false;
		}
		while (index < classString.size() && currentTokenMatches(classString[index], index, token)) {
			++index;
			++prevMatchCount;
			if (prevMatchCount == token.modifier.quantifier.maxCount) {
				return true;
			}
		}
		return prevMatchCount >= token.modifier.quantifier.minCount;
	}

	bool currentTokenMatches(const BreakClassRuleTraits& traits, uint32_t& index, const BreakClassRuleToken& token) {
		if (token.modifier.excludeEastAsianWidths && traits.eastAsianWidth != EastAsianWidth::Other) {
			return token.modifier.negated;
		}
		if (token.modifier.mustBeAtStart && index != 0) {
			return token.modifier.negated;
		}
		if (token.modifier.matchExtendedPictographics) {
			return traits.isExtendedPictographic ^ token.modifier.negated;
		}

		if (token.modifier.optional) {
			for (auto& option : token.classOptions) {
				if (traits.breakClass == option) {
					return !token.modifier.negated;
				}
			}
			return token.modifier.negated;
		} else {
			for (auto& option : token.classOptions) {
				if (traits.breakClass != option) {
					return token.modifier.negated;
				}
			}
			return !token.modifier.negated;
		}
	}

	BreakClassRule parseRule(const std::string_view& rule) {
		BreakClassRule result;

		Parser parser = { .currentState = ParserState::Start };

		for (auto& character : rule) {
			StateAction action = tokenizerFSM[static_cast<uint32_t>(parser.currentState)][inputSymbolClass(character)];
			parser.currentState = action.newState;
			if (action.action == StackAction::Push) {
				if (!parser.stack.empty() && (parser.stack.back().symbolClass == inputSymbolClass(character) ||
											  // Collapse and/or ignore whitespaces
											  parser.stack.back().symbolClass == InputSymbolClass::Whitespace)) {
					parser.stack.back().symbolClass = inputSymbolClass(character);
					if (inputSymbolClass(character) != InputSymbolClass::Whitespace)
						parser.stack.back().text.push_back(character);
				} else {
					parser.stack.push_back({ .symbolClass = inputSymbolClass(character),
											 .text = (inputSymbolClass(character) == InputSymbolClass::Whitespace)
														 ? std::string("")
														 : std::string({ character }) });
				}
			}
			if (parser.currentState == ParserState::Error) {
				logError("Syntax error parsing rule %s\n", rule);
				return {};
			}
		}

		bool insideTokenGroup = false;

		for (auto iterator = parser.stack.begin(); iterator != parser.stack.end(); ++iterator) {
			switch (iterator->symbolClass) {
				case InputSymbolClass::Modifier:
					for (auto& character : iterator->text) {
						switch (character) {
							case '*':
								if (result.tokens.back().modifier.quantifier.minCount != 1U ||
									result.tokens.back().modifier.quantifier.maxCount != 1U) {
									logError("Error parsing rule %s: More than one quantifier per token!\n", rule);
									return {};
								}
								result.tokens.back().modifier.quantifier.minCount = 0;
								result.tokens.back().modifier.quantifier.maxCount = ~0U;
								break;
							case '?':
								if (result.tokens.back().modifier.quantifier.minCount != 1U ||
									result.tokens.back().modifier.quantifier.maxCount != 1U) {
									logError("Error parsing rule %s: More than one quantifier per token!\n", rule);
									return {};
								}
								result.tokens.back().modifier.quantifier.minCount = 0;
								result.tokens.back().modifier.quantifier.maxCount = ~0U;
								break;
							case '^':
								result.tokens.back().modifier.negated = true;
								break;
							case '_':
								result.tokens.back().modifier.excludeEastAsianWidths = true;
								break;
							case '<':
								result.tokens.back().modifier.mustBeAtStart = true;
								break;
							case 'p':
								result.tokens.back().modifier.matchExtendedPictographics = true;
								break;
						}
					}
					break;
				case InputSymbolClass::CloseParenthesis:
					insideTokenGroup = false;
					break;
				case InputSymbolClass::VerticalBar:
					result.tokens.back().modifier.optional = true;
					break;
				case InputSymbolClass::Class:
					if (insideTokenGroup) {
						result.tokens.back().classOptions.push_back(breakClassFromString(iterator->text));
					} else {
						result.tokens.push_back({ .classOptions = { breakClassFromString(iterator->text) } });
					}
					break;
				case InputSymbolClass::OpenParenthesis:
					result.tokens.push_back({ .classOptions = {} });
					insideTokenGroup = true;
					break;
				case InputSymbolClass::BreakAction:
					switch (iterator->text[0]) {
						case 'x':
							result.action = { .newStatus = LinebreakStatus::DoNotBreak,
											  .beforeIndex = static_cast<uint32_t>(result.tokens.size()) };
							break;
						case '%':
							result.action = { .newStatus = LinebreakStatus::Opportunity,
											  .beforeIndex = static_cast<uint32_t>(result.tokens.size()) };
							break;
						case '!':
							result.action = { .newStatus = LinebreakStatus::Mandatory,
											  .beforeIndex = static_cast<uint32_t>(result.tokens.size()) };
							break;
					}
				default:
					break;
			}
		}

		result.isValid = true;
		return result;
	}

	InputSymbolClass inputSymbolClass(char inputSymbol) {
		switch (inputSymbol) {
			case 'x':
			case '%':
			case '!':
				return InputSymbolClass::BreakAction;
			case '*':
			case '?':
			case '^':
			case '_':
			case '<':
			case 'p':
				return InputSymbolClass::Modifier;
			case '|':
				return InputSymbolClass::VerticalBar;
			case ' ':
				return InputSymbolClass::Whitespace;
			case '(':
				return InputSymbolClass::OpenParenthesis;
			case ')':
				return InputSymbolClass::CloseParenthesis;
			default:
				return InputSymbolClass::Class;
				break;
		}
	}
} // namespace vanadium::ui
