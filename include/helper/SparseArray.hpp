#include <Log.hpp>
#include <algorithm>
#include <optional>
#include <vector>

namespace vanadium {
	template <typename T> struct SparseArrayElement {
		size_t index;
		T data;
	};

	template <typename T> class SparseArray {
	  public:
		SparseArray() {}
		SparseArray(size_t size);

		// If the element is already present, it is replaced.
		void insert(size_t index, const T& element);
		// If the element is already present, it is replaced.
		void insert(size_t index, T&& element);

		std::optional<T> tryAt(size_t index) const;

		const T& at(size_t index) const;
		const T& operator[](size_t index) const;

		T& at(size_t index);
		T& operator[](size_t index);

		void erase(size_t index);

	  private:
		bool compareIndex(const SparseArrayElement<T>& one, const SparseArrayElement<T>& other) {
			return one.index < other.index;
		}

		std::vector<SparseArrayElement> m_elements;
	};

	template <typename T> SparseArray<T>::SparseArray(size_t size) { m_elements.reserve(size); }

	template <typename T> void SparseArray<T>::insert(size_t index, const T& element) {
		auto insertLocIterator = std::lower_bound(m_elements.begin(), m_elements.end(), compareIndex);
		if (insertLocIterator == m_elements.end()) {
			m_elements.push_back({ .index = index, .data = element });
		} else {
			if (insertLocIterator->index == index) {
				insertLocIterator->data = element;
			} else {
				m_elements.insert(insertLocIterator, { .index = index, .data = element });
			}
		}
	}

	template <typename T> void SparseArray<T>::insert(size_t index, T&& element) {
		auto insertLocIterator = std::lower_bound(m_elements.begin(), m_elements.end(), compareIndex);
		if (insertLocIterator == m_elements.end()) {
			m_elements.push_back({ .index = index, .data = std::forward<T>(element) });
		} else {
			if (insertLocIterator->index == index) {
				insertLocIterator->data = std::forward<T>(element);
			} else {
				m_elements.insert(insertLocIterator, { .index = index, .data = std::forward<T>(element) });
			}
		}
	}

	template <typename T> const T& SparseArray<T>::at(size_t index) const {
		auto locIterator = std::lower_bound(m_elements.cbegin(), m_elements.ecnd(), compareIndex);
		assertFatal(locIterator->index == index, "SparseArray: Out-of-bounds access caught!");
		return locIterator->data;
	}

	template <typename T> const T& SparseArray<T>::operator[](size_t index) const { return at(index); }

	template<typename T> T& SparseArray<T>::at(size_t index) {
		auto locIterator = std::lower_bound(m_elements.begin(), m_elements.end(), compareIndex);
		assertFatal(locIterator->index == index, "SparseArray: Out-of-bounds access caught!");
		return locIterator->data;
	}

	template<typename T> T& SparseArray<T>::operator[](size_t index) {
		return at(index);
	}

} // namespace vanadium