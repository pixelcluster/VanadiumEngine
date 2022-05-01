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
					bool lastTokenMatched = false;

					while (j < classString.size() && tokenIndex < rule.tokens.size()) {
						if (tryMatchToken(classString, matchesInARow, j, rule.tokens[tokenIndex])) {
							if (tokenIndex == rule.tokens.size() - 1) {
								lastTokenMatched = true;
							}
						}
						matchesInARow = 0;
						++tokenIndex;
					}

					if (lastTokenMatched) {
						matchIndices.push_back(i);
					}
				}
				++i;
			}
		}

		for (auto index : matchIndices) {
			if (index + rule.action.beforeIndex - 1U < statusArray.size()) {
				if (statusArray[index + rule.action.beforeIndex - 1U] == LinebreakStatus::Undefined) {
					statusArray[index + rule.action.beforeIndex - 1U] = rule.action.newStatus;
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

		if (token.modifier.allOptionsRequired) {
			for (auto& option : token.classOptions) {
				if (traits.breakClass != option) {
					return token.modifier.negated;
				}
			}
			index += token.classOptions.size();
			return !token.modifier.negated;
		} else {
			for (auto& option : token.classOptions) {
				if (traits.breakClass == option) {
					return !token.modifier.negated;
				}
			}
			return token.modifier.negated;
		}
	}

	BreakClassRule parseRule(const std::string_view& rule) {
		BreakClassRule result;

		BreakClassRuleModifier currentTokenModifier;
		std::string lastParsedTokenName = "";
		lastParsedTokenName.reserve(3);
		size_t index = 0;

		bool insideTokenGroup = false;
		while (index < rule.size()) {

			// scan for modifiers appearing before/after a token

			ignoreWhitespace(rule, index);
			BreakClassRuleModifier newTokenModifier = parseModifiers(rule, index);
			if (!newTokenModifier.valid)
				return {};
			if (insideTokenGroup) {
				if (!currentTokenModifier.negated && newTokenModifier.negated) {
					logError("Error: Rule %s is invalid: Unexpected \"^\"!\n", rule.data());
					return {};
				}
				if (!currentTokenModifier.excludeEastAsianWidths && newTokenModifier.excludeEastAsianWidths) {
					logError("Error: Rule %s is invalid: Unexpected \"_\"!\n", rule.data());
					return {};
				}
				if (!currentTokenModifier.matchExtendedPictographics && newTokenModifier.matchExtendedPictographics) {
					logError("Error: Rule %s is invalid: Unexpected \"p\"!\n", rule.data());
					return {};
				}
				if (!currentTokenModifier.mustBeAtStart && newTokenModifier.mustBeAtStart) {
					logError("Error: Rule %s is invalid: Unexpected \"<\"!\n", rule.data());
					return {};
				}
			}
			currentTokenModifier.negated |= newTokenModifier.negated;
			currentTokenModifier.excludeEastAsianWidths |= newTokenModifier.excludeEastAsianWidths;
			currentTokenModifier.matchExtendedPictographics |= newTokenModifier.matchExtendedPictographics;
			currentTokenModifier.mustBeAtStart |= newTokenModifier.mustBeAtStart;
			currentTokenModifier.allOptionsRequired &= newTokenModifier.allOptionsRequired;
			if (currentTokenModifier.quantifier.minCount != 1U && currentTokenModifier.quantifier.maxCount != 1U &&
				newTokenModifier.quantifier.minCount != 1U && newTokenModifier.quantifier.maxCount != 1U) {
				logError("Error: Rule %s is invalid: More than one quantifier per token!\n", rule.data());
				return {};
			}
			currentTokenModifier.quantifier = newTokenModifier.quantifier;

			// all modifiers before/after the current token were collected, add it

			if (!lastParsedTokenName.empty()) {
				if (insideTokenGroup) {
					result.tokens.back().classOptions.push_back(breakClassFromString(lastParsedTokenName));
				} else {
					result.tokens.push_back({ .classOptions = { breakClassFromString(lastParsedTokenName) },
											  .modifier = currentTokenModifier });
					currentTokenModifier = {};
				}
				lastParsedTokenName.clear();
			}

			// Scan for token group markers

			ignoreWhitespace(rule, index);
			if (rule[index] == '(') {
				if (insideTokenGroup) {
					logError("Error: Rule %s is invalid: You cannot nest parentheses!\n", rule.data());
					return {};
				}
				insideTokenGroup = true;
				ignoreWhitespace(rule, index);
				currentTokenModifier = parseModifiers(rule, index);
				if (!newTokenModifier.valid)
					return {};
				result.tokens.push_back({});
				++index;
			} else if (rule[index] == ')') {
				if (!insideTokenGroup || result.tokens.empty()) {
					logError("Error: Rule %s is invalid: Unexpected \")\"!\n", rule.data());
					return {};
				}
				BreakClassRuleModifier newTokenModifier = parseModifiers(rule, index);
				if (!newTokenModifier.valid)
					return {};
				currentTokenModifier.negated |= newTokenModifier.negated;
				currentTokenModifier.excludeEastAsianWidths |= newTokenModifier.excludeEastAsianWidths;
				currentTokenModifier.matchExtendedPictographics |= newTokenModifier.matchExtendedPictographics;
				currentTokenModifier.mustBeAtStart |= newTokenModifier.mustBeAtStart;
				currentTokenModifier.allOptionsRequired &= newTokenModifier.allOptionsRequired;
				if (currentTokenModifier.quantifier.minCount != 1U && currentTokenModifier.quantifier.maxCount != 1U &&
					newTokenModifier.quantifier.minCount != 1U && newTokenModifier.quantifier.maxCount != 1U) {
					logError("Error: Rule %s is invalid: More than one quantifier per token!\n", rule.data());
					return {};
				}
				currentTokenModifier.quantifier = newTokenModifier.quantifier;
				result.tokens.back().modifier = currentTokenModifier;
				currentTokenModifier = {};
				insideTokenGroup = false;
				++index;
			}

			// scan for break actions

			ignoreWhitespace(rule, index);

			if (isBreakAction(rule[index])) {
				if (result.action.newStatus != LinebreakStatus::Undefined) {
					logError("Error: Rule %s is invalid: More than one break status modification per rule!\n",
							 rule.data());
					return {};
				}
				switch (rule[index]) {
					case 'x':
						result.action.newStatus = LinebreakStatus::DoNotBreak;
						result.action.beforeIndex = index;
						break;
					case '%':
						result.action.newStatus = LinebreakStatus::Opportunity;
						result.action.beforeIndex = index;
						break;
					case '!':
						result.action.newStatus = LinebreakStatus::Mandatory;
						result.action.beforeIndex = index;
						break;
				}
				++index;
			}

			// parse new token, if present

			ignoreWhitespace(rule, index);
			while (!isModifier(rule[index]) && !isBreakAction(rule[index]) && rule[index] != ' ' &&
				   rule[index] != '(' && rule[index] != ')' && index < rule.size()) {
				lastParsedTokenName.push_back(rule[index]);
				++index;
			}
		}

		if (!lastParsedTokenName.empty()) {
			result.tokens.push_back(
				{ .classOptions = { breakClassFromString(lastParsedTokenName) }, .modifier = currentTokenModifier });
			currentTokenModifier = {};
			lastParsedTokenName.clear();
		}

		result.isValid = true;
		return result;
	}

	BreakClassRuleModifier parseModifiers(const std::string_view& rule, size_t& index) {
		BreakClassRuleModifier modifier;
		while (index < rule.size() && isModifier(rule[index])) {
			char character = rule[index];
			switch (character) {
				case '*':
					if (modifier.quantifier.minCount != 1U || modifier.quantifier.maxCount != 1U) {
						logError("Error: Rule %s is invalid: More than one quantifier per token!\n", rule.data());
						modifier.valid = false;
						return modifier;
					}
					modifier.quantifier.minCount = 0U;
					modifier.quantifier.maxCount = ~0U;
					break;
				case '?':
					if (modifier.quantifier.minCount != 1U || modifier.quantifier.maxCount != 1U) {
						logError("Error: Rule %s is invalid: More than one quantifier per token!\n", rule.data());
						modifier.valid = false;
						return modifier;
					}
					modifier.quantifier.minCount = 0U;
					modifier.quantifier.maxCount = 1U;
					break;
				case '^':
					modifier.negated = true;
					break;
				case '_':
					modifier.excludeEastAsianWidths = true;
					break;
				case '<':
					modifier.mustBeAtStart = true;
					break;
				case 'p':
					modifier.matchExtendedPictographics = true;
					break;
				case '|':
					modifier.allOptionsRequired = false;
					break;
			}
			++index;
		}
		return modifier;
	}

	bool isModifier(char character) {
		switch (character) {
			case '*':
			case '?':
			case '^':
			case '_':
			case '<':
			case 'p':
			case '|':
				return true;
			default:
				return false;
		}
	}

	bool isBreakAction(char character) {
		switch (character) {
			case 'x':
			case '%':
			case '!':
				return true;
			default:
				return false;
		}
	}

	char peekNext(const std::string_view& rule, size_t index) {
		if (index < rule.size() - 1) {
			return rule[index + 1];
		}
		return '\0';
	}

	void ignoreWhitespace(const std::string_view& rule, size_t& index) {
		while (index < rule.size() && rule[index] == ' ')
			++index;
	}
} // namespace vanadium::ui
