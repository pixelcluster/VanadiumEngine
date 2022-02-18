#pragma once

#include <concepts>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

template <std::floating_point F>
void testFloatEqualWithError(F expected, F actual, const std::string_view& message, F error = 0.0001f) {
	if (abs(expected - actual) > error) {
		std::cout << "Test failed: Float value was expected to be " << expected << ", but was actually " << actual
				  << "! Message: " << message << "\n";
		std::exit(1);
	}
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
		std::cout << "Test failed: Value was expected to be less than " << expected << ", but was actually " << actual
				  << "! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::less<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testLess(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected < actual)) {
		std::cout << "Test failed: A value was not less than than the expected value! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::less_equal<T>>&& CanCout<T>) void testLessEqual(
	const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected <= actual)) {
		std::cout << "Test failed: Value was expected to be less than or equal to " << expected << ", but was actually "
				  << actual << "! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::less_equal<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testLessEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected <= actual)) {
		std::cout << "Test failed: A value was not less than or equal to the expected value! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>>&& CanCout<T>) void testEqual(const T& expected, const T& actual,
																				 const std::string_view& message) {
	if (!(expected == actual)) {
		std::cout << "Test failed: Value was expected to be equal to " << expected << ", but was actually " << actual
				  << "! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected == actual)) {
		std::cout << "Test failed: A value was not equal to the expected value! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>>&& CanCout<T>) void testNotEqual(const T& expected, const T& actual,
																					const std::string_view& message) {
	if (expected == actual) {
		std::cout << "Test failed: Value was expected to be not equal to " << expected << ", but was actually "
				  << actual << "! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::equal_to<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testNotEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (expected == actual) {
		std::cout << "Test failed: A value was equal to the expected value! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::greater_equal<T>>&& CanCout<T>) void testGreaterEqual(
	const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cout << "Test failed: Value was expected to be greater than or equal to " << expected
				  << ", but was actually " << actual << "! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::greater_equal<T>> && !CanCout<T> &&
		 !CanStringify<T>) void testGreaterEqual(const T& expected, const T& actual, const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cout << "Test failed: A value was not greater than or equal to the expected value! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::greater<T>>&& CanCout<T>) void testGreater(const T& expected, const T& actual,
																				  const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cout << "Test failed: Value was expected to be greater than or equal to " << expected
				  << ", but was actually " << actual << "! Message: " << message << "\n";
		std::exit(1);
	}
}

template <typename T>
requires(CanTestWithComparator<T, std::greater<T>> && !CanCout<T>) void testGreater(const T& expected, const T& actual,
																					const std::string_view& message) {
	if (!(expected >= actual)) {
		std::cout << "Test failed: A value was not greater than the expected value! Message: " << message << "\n";
		std::exit(1);
	}
}