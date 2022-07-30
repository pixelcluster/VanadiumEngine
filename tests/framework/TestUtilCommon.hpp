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

#include <cmath>
#include <concepts>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

template <std::floating_point F>
void testFloatEqualWithError(F expected, F actual, const std::string_view& message, F errorPercent = 0.001f) {
	float actualErrorPercent = fabs(expected - actual) / expected * 100.0f;
	if (actualErrorPercent > errorPercent) {
		std::cerr << "Test failed: Float value was expected to be " << expected << ", but was actually " << actual
				  << " (" << actualErrorPercent << "% error)! Message : " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Floating-point equality check with message \"" << message << "\" passed, error was "
				  << actualErrorPercent << "%.\n";
	}
#endif
}

template <typename T>
concept CanCout = requires(T t) {
	std::cout << t;
};

template <typename T>
concept CanStringify = requires(T t) {
	std::to_string(t);
};

template <typename T, typename Comparator>
concept CanTestWithComparator = requires(T t, T u) {
	Comparator()(t, u);
};

template <typename T>
requires(CanTestWithComparator<T, std::less<T>>&& CanCout<T>) void testLess(const T& expected, const T& actual,
																			const std::string_view& message) {
	if (!(expected < actual)) {
		std::cerr << "Test failed: Value was expected to be less than " << expected << ", but was actually " << actual
				  << "! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Less check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::less<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testLess(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected < actual)) {
		std::cerr << "Test failed: A value was not less than than the expected value! Message: " << message
				  << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Less check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::less_equal<T>>&& CanCout<T>) void testLessEqual(
	const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected <= actual)) {
		std::cerr << "Test failed: Value was expected to be less than or equal to " << expected << ", but was actually "
				  << actual << "! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Less or equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::less_equal<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testLessEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected <= actual)) {
		std::cerr << "Test failed: A value was not less than or equal to the expected value! Message: " << message
				  << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Less or equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>>&& CanCout<T>) void testEqual(const T& expected, const T& actual,
																				 const std::string_view& message) {
	if (!(expected == actual)) {
		std::cerr << "Test failed: Value was expected to be equal to " << expected << ", but was actually " << actual
				  << "! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected == actual)) {
		std::cerr << "Test failed: A value was not equal to the expected value! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>>&& CanCout<T>) void testNotEqual(const T& expected, const T& actual,
																					const std::string_view& message) {
	if (expected == actual) {
		std::cerr << "Test failed: Value was expected to be not equal to " << expected << ", but was actually "
				  << actual << "! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Not equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testNotEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (expected == actual) {
		std::cerr << "Test failed: A value was equal to the expected value! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Not equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::greater_equal<T>>&& CanCout<T>) void testGreaterEqual(
	const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cerr << "Test failed: Value was expected to be greater than or equal to " << expected
				  << ", but was actually " << actual << "! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Greater or equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::greater_equal<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testGreaterEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cerr << "Test failed: A value was not greater than or equal to the expected value! Message: " << message
				  << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Greater or equal check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::greater<T>>&& CanCout<T>) void testGreater(const T& expected, const T& actual,
																				  const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cerr << "Test failed: Value was expected to be greater than or equal to " << expected
				  << ", but was actually " << actual << "! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Greater check with message \"" << message << "\" passed.\n";
	}
#endif
}

template <typename T>
requires(CanTestWithComparator<T, std::greater<T>> && !CanCout<T>) void testGreater(const T& expected, const T& actual,
																					const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cerr << "Test failed: A value was not greater than the expected value! Message: " << message << std::endl;
		std::exit(1);
	}
#ifdef TEST_TRACE_PASSED
	else {
		std::cout << "Greater check with message \"" << message << "\" passed.\n";
	}
#endif
}
