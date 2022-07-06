#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <new>
#include <type_traits>

namespace vanadium {
	template <typename T>
	concept HasDefaultConstructor = requires() {
		{ T() };
	};

	template <typename T>
	concept HasEqualsOperator = requires(const T& t, const T& u) {
		{ t == u } -> std::convertible_to<bool>;
	};

	template <typename T>
	concept HasNotEqualsOperator = requires(const T& t, const T& u) {
		{ t != u } -> std::convertible_to<bool>;
	};

	template <typename T, typename U>
	concept InsertIterator = requires(T& t) {
		{ ++t } -> std::same_as<T&>;
		{ *t };
	}
	&&HasNotEqualsOperator<T>;

	template <typename T> class SimpleVector {
	  public:
		template <typename value> class IteratorBase {
		  public:
			using value_type = value;
			using difference_type = ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;

			IteratorBase(pointer element) : m_element(element) {}

			reference operator*() { return *this->m_element; }
			pointer operator->() { return this->m_element; }
			const reference operator*() const { return *this->m_element; }
			const pointer operator->() const { return this->m_element; }
			bool operator==(const IteratorBase& other) const { return this->m_element == other.m_element; }
			bool operator!=(const IteratorBase& other) const { return this->m_element != other.m_element; }

		  protected:
			pointer m_element;
		};
		template <typename value> class Iterator : public IteratorBase<value> {
		  public:
			using value_type = value;
			using difference_type = ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;

			Iterator(pointer element) : IteratorBase<value>(element) {}

			Iterator& operator++();
			Iterator operator++(int);
			Iterator& operator--();
			Iterator operator--(int);
			Iterator operator+(ptrdiff_t n) const { return Iterator(this->m_element + n); }
			Iterator operator-(ptrdiff_t n) const { return Iterator(this->m_element - n); }
			ptrdiff_t operator-(const Iterator& other) const { return this->m_element - other.m_element; }
			Iterator& operator+=(ptrdiff_t n);
			Iterator& operator-=(ptrdiff_t n);
			bool operator>=(const Iterator& other) const;
			bool operator>(const Iterator& other) const;
			bool operator<=(const Iterator& other) const;
			bool operator<(const Iterator& other) const;
			reference operator[](size_t n) { return *(this->m_element + n); }
		};
		template <typename value> class ReverseIterator : public IteratorBase<value> {
		  public:
			using value_type = value;
			using difference_type = ptrdiff_t;
			using reference = value_type&;
			using pointer = value_type*;

			ReverseIterator(pointer element) : IteratorBase<value>(element) {}

			ReverseIterator& operator++();
			ReverseIterator operator++(int);
			ReverseIterator& operator--();
			ReverseIterator operator--(int);
			ReverseIterator operator+(ptrdiff_t n) const { return ReverseIterator(this->m_element - n); }
			ReverseIterator operator-(ptrdiff_t n) const { return ReverseIterator(this->m_element + n); }
			ptrdiff_t operator-(const ReverseIterator& other) const { return other.m_element - this->m_element; }
			ReverseIterator& operator+=(ptrdiff_t n);
			ReverseIterator& operator-=(ptrdiff_t n);
			bool operator>=(const ReverseIterator& other) const;
			bool operator>(const ReverseIterator& other) const;
			bool operator<=(const ReverseIterator& other) const;
			bool operator<(const ReverseIterator& other) const;
			reference operator[](size_t n) { return *(this->m_element - n); }
		};

		using iterator = Iterator<T>;
		using const_iterator = Iterator<const T>;
		using reverse_iterator = ReverseIterator<T>;
		using const_reverse_iterator = ReverseIterator<const T>;

		SimpleVector(size_t size, const T& element);
		SimpleVector(size_t size) requires(HasDefaultConstructor<T>);
		SimpleVector(std::initializer_list<T> list) requires(std::is_trivially_copyable_v<T>);
		SimpleVector(std::initializer_list<T> list) requires(!std::is_trivially_copyable_v<T>);
		SimpleVector() = default;
		SimpleVector(const SimpleVector& other);
		SimpleVector& operator=(const SimpleVector& other);
		SimpleVector(SimpleVector&& other);
		SimpleVector& operator=(SimpleVector&& other);
		~SimpleVector();

		bool operator==(const SimpleVector& other) const requires(HasEqualsOperator<T>);
		bool operator!=(const SimpleVector& other) const requires(HasEqualsOperator<T>);

		void push_back(const T& element);
		void push_back(T&& element);

		void pop_back();

		const T& back() const { return m_data[m_size - 1]; }
		T& back() { return m_data[m_size - 1]; }

		void resize(size_t size) requires(HasDefaultConstructor<T>);
		void resize(size_t size, const T& element);

		void reserve(size_t capacity);

		iterator begin() { return iterator(m_data); }
		iterator end() { return iterator(m_data + m_size); }
		const_iterator begin() const { return const_iterator(m_data); }
		const_iterator end() const { return const_iterator(m_data + m_size); }
		reverse_iterator rbegin() { return reverse_iterator(m_data + m_size - 1); }
		reverse_iterator rend() { return reverse_iterator(m_data - 1); }

		const_iterator cbegin() const { return const_iterator(m_data); }
		const_iterator cend() const { return const_iterator(m_data + m_size); }
		const_reverse_iterator crbegin() const { return const_reverse_iterator(m_data + m_size - 1); }
		const_reverse_iterator crend() const { return const_reverse_iterator(m_data - 1); }

		void insert(const iterator& at, const T& element);
		void insert(const iterator& at, T&& element);
		void insert(size_t index, const T& element);
		void insert(size_t index, T&& element);
		template <InsertIterator<T> Iterator> void insert(const iterator& at, Iterator begin, Iterator end);

		void erase(const iterator& at) { shiftBackward(at - begin()); }
		void erase(size_t index) { shiftBackward(index); }

		size_t size() const { return m_size; }

		T& operator[](size_t index) { return m_data[index]; }
		const T& operator[](size_t index) const { return m_data[index]; }

		T* data() { return m_data; }
		const T* data() const { return m_data; }
		void clear() { shrink(0); }

		bool empty() const { return m_size == 0; }

	  private:
		size_t max(size_t a, size_t b) { return a > b ? a : b; }

		void copy(T* dst) const requires(std::is_trivially_copyable_v<T>);
		void copy(T* dst) const requires(!std::is_trivially_copyable_v<T>);
		void move(T* dst) requires(std::is_trivially_copyable_v<T>);
		void move(T* dst) requires(!std::is_trivially_copyable_v<T>);
		// Shifts all elements from [index; size] forward by one
		void shiftForward(size_t index) requires(std::is_trivially_move_assignable_v<T>);
		void shiftForward(size_t index) requires(!std::is_trivially_move_assignable_v<T>);
		// Shifts all elements from [index + 1; size] back by one
		void shiftBackward(size_t index) requires(std::is_trivially_move_assignable_v<T>);
		void shiftBackward(size_t index) requires(!std::is_trivially_move_assignable_v<T>);
		void shrink(size_t newsize) requires(std::is_trivially_destructible_v<T>);
		void shrink(size_t newsize) requires(!std::is_trivially_destructible_v<T>);

		void reallocate(size_t minSize);

		T* m_data = nullptr;
		size_t m_size = 0;
		size_t m_capacity = 0;
	};

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template Iterator<value_type>& SimpleVector<T>::Iterator<value_type>::operator++() {
		++this->m_element;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template Iterator<value_type> SimpleVector<T>::Iterator<value_type>::operator++(int) {
		auto iter = *this;
		++this->m_element;
		return iter;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template Iterator<value_type>& SimpleVector<T>::Iterator<value_type>::operator--() {
		--this->m_element;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template Iterator<value_type> SimpleVector<T>::Iterator<value_type>::operator--(int) {
		auto iter = *this;
		--this->m_element;
		return iter;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template Iterator<value_type>& SimpleVector<T>::Iterator<value_type>::operator+=(
		ptrdiff_t n) {
		this->m_element += n;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template Iterator<value_type>& SimpleVector<T>::Iterator<value_type>::operator-=(
		ptrdiff_t n) {
		this->m_element -= n;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::Iterator<value_type>::operator>=(
		const typename SimpleVector<T>::Iterator<value_type>& other) const {
		return this->m_element >= other.m_element;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::Iterator<value_type>::operator>(
		const typename SimpleVector<T>::Iterator<value_type>& other) const {
		return this->m_element > other.m_element;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::Iterator<value_type>::operator<=(
		const typename SimpleVector<T>::Iterator<value_type>& other) const {
		return this->m_element <= other.m_element;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::Iterator<value_type>::operator<(
		const typename SimpleVector<T>::Iterator<value_type>& other) const {
		return this->m_element < other.m_element;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template ReverseIterator<value_type>& SimpleVector<T>::ReverseIterator<
		value_type>::operator++() {
		--this->m_element;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template ReverseIterator<value_type> SimpleVector<T>::ReverseIterator<
		value_type>::operator++(int) {
		auto iter = *this;
		--this->m_element;
		return iter;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template ReverseIterator<value_type>& SimpleVector<T>::ReverseIterator<
		value_type>::operator--() {
		++this->m_element;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template ReverseIterator<value_type> SimpleVector<T>::ReverseIterator<
		value_type>::operator--(int) {
		auto iter = *this;
		++this->m_element;
		return iter;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template ReverseIterator<value_type>& SimpleVector<T>::ReverseIterator<
		value_type>::operator+=(ptrdiff_t n) {
		this->m_element -= n;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	typename SimpleVector<T>::template ReverseIterator<value_type>& SimpleVector<T>::ReverseIterator<
		value_type>::operator-=(ptrdiff_t n) {
		this->m_element += n;
		return *this;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::ReverseIterator<value_type>::operator>=(
		const typename SimpleVector<T>::ReverseIterator<value_type>& other) const {
		return this->m_element <= other.m_element;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::ReverseIterator<value_type>::operator>(
		const typename SimpleVector<T>::ReverseIterator<value_type>& other) const {
		return this->m_element < other.m_element;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::ReverseIterator<value_type>::operator<=(
		const typename SimpleVector<T>::ReverseIterator<value_type>& other) const {
		return this->m_element >= other.m_element;
	}

	template <typename T>
	template <typename value_type>
	bool SimpleVector<T>::ReverseIterator<value_type>::operator<(
		const typename SimpleVector<T>::ReverseIterator<value_type>& other) const {
		return this->m_element > other.m_element;
	}

	template <typename T> SimpleVector<T>::SimpleVector(size_t size, const T& element) {
		m_capacity = m_size = size;
		m_data = reinterpret_cast<T*>(std::malloc(size * sizeof(T)));
		for (size_t i = 0; i < size; ++i) {
			new (m_data + i) T(element);
		}
	}

	template <typename T> SimpleVector<T>::SimpleVector(size_t size) requires(HasDefaultConstructor<T>) {
		m_capacity = m_size = size;
		m_data = reinterpret_cast<T*>(std::malloc(size * sizeof(T)));
		if constexpr (!std::is_trivially_constructible_v<T>) {
			for (size_t i = 0; i < size; ++i) {
				new (m_data + i) T();
			}
		}
	}

	template <typename T>
	SimpleVector<T>::SimpleVector(std::initializer_list<T> list) requires(std::is_trivially_copyable_v<T>)
		: m_size(0), m_capacity(0) {
		reallocate(list.size());
		if (list.size())
			std::memcpy(m_data, list.begin(), list.size() * sizeof(T));
		m_size = list.size();
	}
	template <typename T>
	SimpleVector<T>::SimpleVector(std::initializer_list<T> list) requires(!std::is_trivially_copyable_v<T>)
		: m_size(0), m_capacity(0) {
		reallocate(list.size());
		size_t i = 0;
		for (const auto& element : list) {
			new (m_data + i) T(element);
			++i;
		}
		m_size = list.size();
	}

	template <typename T> SimpleVector<T>::SimpleVector(const SimpleVector<T>& other) : m_size(0), m_capacity(0) {
		reallocate(other.m_size);
		m_size = other.m_size;
		other.copy(m_data);
	}

	template <typename T> SimpleVector<T>& SimpleVector<T>::operator=(const SimpleVector<T>& other) {
		if (&other != this) {
			reallocate(other.m_size);
			m_size = other.m_size;
			other.copy(m_data);
		}
		return *this;
	}

	template <typename T>
	SimpleVector<T>::SimpleVector(SimpleVector<T>&& other)
		: m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
		other.m_capacity = 0;
		other.m_data = nullptr;
		other.m_size = 0;
	}

	template <typename T> SimpleVector<T>& SimpleVector<T>::operator=(SimpleVector<T>&& other) {
		if (&other != this) {
			m_data = other.m_data;
			m_size = other.m_size;
			m_capacity = other.m_capacity;
			other.m_capacity = 0;
			other.m_data = nullptr;
			other.m_size = 0;
		}
		return *this;
	}

	template <typename T> SimpleVector<T>::~SimpleVector() {
		for (size_t i = 0; i < m_size; ++i) {
			m_data[i].~T();
		}
		if (m_data)
			std::free(m_data);
		m_data = nullptr;
	}

	template <typename T>
	bool SimpleVector<T>::operator==(const SimpleVector& other) const requires(HasEqualsOperator<T>) {
		if (other.size() != m_size)
			return false;

		for (size_t i = 0; i < m_size; ++i) {
			if (!(m_data[i] == other[i]))
				return false;
		}
		return true;
	}

	template <typename T>
	bool SimpleVector<T>::operator!=(const SimpleVector& other) const requires(HasEqualsOperator<T>) {
		if (other.size() != m_size)
			return false;

		for (size_t i = 0; i < m_size; ++i) {
			if (!(m_data[i] != other[i]))
				return false;
		}
		return true;
	}

	template <typename T> void SimpleVector<T>::push_back(const T& element) {
		reallocate(m_size + 1);
		new (m_data + m_size++) T(element);
	}

	template <typename T> void SimpleVector<T>::push_back(T&& element) {
		reallocate(m_size + 1);
		new (m_data + m_size++) T(static_cast<T&&>(element));
	}

	template <typename T> void SimpleVector<T>::pop_back() { m_data[--m_size].~T(); }

	template <typename T> void SimpleVector<T>::resize(size_t size) requires(HasDefaultConstructor<T>) {
		if (size < m_size) {
			shrink(size);
		} else {
			size_t oldSize = m_size;
			m_size = size;
			reallocate(size);
			for (size_t i = oldSize; i < m_size; ++i) {
				new (m_data + i) T();
			}
		}
	}

	template <typename T> void SimpleVector<T>::resize(size_t size, const T& element) {
		if (size < m_size) {
			shrink(size);
		} else {
			size_t oldSize = m_size;
			m_size = size;
			reallocate(size);
			for (size_t i = oldSize; i < m_size; ++i) {
				new (m_data + i) T(element);
			}
		}
	}

	template <typename T> void SimpleVector<T>::reserve(size_t size) {
		if (size > m_capacity) {
			reallocate(size);
		}
	}

	template <typename T> void SimpleVector<T>::insert(const typename SimpleVector<T>::iterator& at, const T& element) {
		size_t index = at - begin();
		shiftForward(index);
		m_data[index] = element;
	}
	template <typename T> void SimpleVector<T>::insert(const typename SimpleVector<T>::iterator& at, T&& element) {
		size_t index = at - begin();
		shiftForward(index);
		m_data[index] = static_cast<T&&>(element);
	}
	template <typename T> void SimpleVector<T>::insert(size_t index, const T& element) {
		shiftForward(index);
		m_data[index] = element;
	}
	template <typename T> void SimpleVector<T>::insert(size_t index, T&& element) {
		shiftForward(index);
		m_data[index] = static_cast<T&&>(element);
	}
	template <typename T>
	template <InsertIterator<T> InIterator>
	void SimpleVector<T>::insert(const iterator& at, InIterator begin, InIterator end) {
		size_t index = at - this->begin();
		while (begin != end) {
			insert(index, *end);
			++index;
			++begin;
		}
	}

	template <typename T> void SimpleVector<T>::copy(T* dst) const requires(std::is_trivially_copyable_v<T>) {
		if (m_size)
			std::memcpy(dst, m_data, m_size * sizeof(T));
	}
	template <typename T> void SimpleVector<T>::copy(T* dst) const requires(!std::is_trivially_copyable_v<T>) {
		for (size_t i = 0; i < m_size; ++i) {
			new (dst + i) T(m_data[i]);
		}
	}

	template <typename T> void SimpleVector<T>::move(T* dst) requires(std::is_trivially_copyable_v<T>) {
		if (m_size)
			std::memcpy(dst, m_data, m_size * sizeof(T));
	}
	template <typename T> void SimpleVector<T>::move(T* dst) requires(!std::is_trivially_copyable_v<T>) {
		for (size_t i = 0; i < m_size; ++i) {
			// std::move without including <utility>
			// https://en.cppreference.com/w/cpp/utility/move:
			//	It is exactly equivalent to a static_cast to an rvalue reference type.
			new (dst + i) T(static_cast<T&&>(m_data[i]));
		}
	}

	template <typename T>
	void SimpleVector<T>::shiftForward(size_t index) requires(std::is_trivially_move_assignable_v<T>) {
		reallocate(m_size);
		std::memmove(m_data + index + 1, m_data + index, (m_size - index) * sizeof(T));
		++m_size;
	}
	template <typename T>
	void SimpleVector<T>::shiftForward(size_t index) requires(!std::is_trivially_move_assignable_v<T>) {
		reallocate(m_size + 1);
		if (m_size >= 1) {
			new (m_data + m_size) T(static_cast<T&&>(m_data[m_size - 1]));

			for (size_t i = m_size - 1; i > index; --i) {
				m_data[i] = static_cast<T&&>(m_data[i - 1]);
			}
		}
		++m_size;
	}

	template <typename T>
	void SimpleVector<T>::shiftBackward(size_t index) requires(std::is_trivially_move_assignable_v<T>) {
		std::memmove(m_data + index, m_data + index + 1, (m_size - index - 1) * sizeof(T));
		--m_size;
	}
	template <typename T>
	void SimpleVector<T>::shiftBackward(size_t index) requires(!std::is_trivially_move_assignable_v<T>) {
		for (size_t i = index + 1; i < m_size; ++i) {
			m_data[i - 1] = static_cast<T&&>(m_data[i]);
		}
		--m_size;
	}

	template <typename T> void SimpleVector<T>::shrink(size_t newsize) requires(std::is_trivially_destructible_v<T>) {
		m_size = newsize;
	}
	template <typename T> void SimpleVector<T>::shrink(size_t newsize) requires(!std::is_trivially_destructible_v<T>) {
		for (size_t i = newsize; i < m_size; ++i) {
			m_data[i].~T();
		}
		m_size = newsize;
	}

	template <typename T> void SimpleVector<T>::reallocate(size_t minSize) {
		if (!minSize || minSize < m_capacity)
			return;
		size_t size = max(size_t(max(m_capacity, 1) * 1.61), minSize);
		T* newData = reinterpret_cast<T*>(std::malloc(size * sizeof(T)));
		if (m_data)
			move(newData);

		size_t oldSize = m_size;
		if (m_data)
			shrink(0);
		m_size = oldSize;

		m_data = newData;
		m_capacity = size;
	}

} // namespace vanadium
