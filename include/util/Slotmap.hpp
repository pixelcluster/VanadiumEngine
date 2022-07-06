#pragma once

#include <algorithm>
#include <cassert>
#include <iterator>
#include <vector>

namespace vanadium {

	using SlotmapHandle = size_t;

	/**
	 *  \brief A slotmap memory structure whose elements can be accessed by unique identifiers.
	 */
	template <typename T> class Slotmap {
	  public:
		class iterator {
		  public:
			// using iterator_category = std::contiguous_iterator_tag;
			using value_type = T;
			using difference_type = ptrdiff_t;
			using reference = T&;
			using pointer = T*;

			iterator() {}

			explicit iterator(pointer elementPointer) : elementPointer(elementPointer) {}

			iterator& operator++() {
				++elementPointer;
				return *this;
			}
			iterator operator++(int32_t) {
				iterator returnValue = *this;
				++elementPointer;
				return returnValue;
			}
			iterator& operator--() {
				--elementPointer;
				return *this;
			}
			iterator operator--(int32_t) {
				iterator returnValue = *this;
				--elementPointer;
				return returnValue;
			}
			iterator& operator+=(size_t amount) {
				elementPointer += amount;
				return *this;
			}
			iterator& operator-=(size_t amount) {
				elementPointer -= amount;
				return *this;
			}
			iterator operator-(size_t amount) const {
				iterator returnValue = iterator(elementPointer);
				returnValue.elementPointer -= amount;
				return returnValue;
			}
			iterator operator+(size_t amount) const {
				iterator returnValue = iterator(elementPointer);
				returnValue.elementPointer += amount;
				return returnValue;
			}
			difference_type operator-(const iterator& other) const { return elementPointer - other.elementPointer; }
			difference_type operator+(const iterator& other) const { return elementPointer + other.elementPointer; }

			bool operator==(iterator other) const { return elementPointer == other.elementPointer; }

			bool operator!=(iterator other) const { return elementPointer != other.elementPointer; }

			reference operator*() const { return *elementPointer; }
			pointer operator->() { return elementPointer; }
			const pointer operator->() const { return elementPointer; }

			operator pointer() const { return elementPointer; }

		  private:
			pointer elementPointer;
		};

		class const_iterator {
		  public:
			// using iterator_category = std::contiguous_iterator_tag;
			using value_type = T;
			using difference_type = ptrdiff_t;
			using reference = const T&;
			using pointer = const T*;

			const_iterator() {}

			explicit const_iterator(pointer elementPointer) : elementPointer(elementPointer) {}

			const_iterator& operator++() {
				++elementPointer;
				return *this;
			}
			const_iterator operator++(int32_t) {
				const_iterator returnValue = *this;
				++elementPointer;
				return returnValue;
			}
			const_iterator& operator--() {
				--elementPointer;
				return *this;
			}
			const_iterator operator--(int32_t) {
				const_iterator returnValue = *this;
				--elementPointer;
				return returnValue;
			}
			const_iterator& operator+=(size_t amount) {
				elementPointer += amount;
				return *this;
			}
			const_iterator& operator-=(size_t amount) {
				elementPointer -= amount;
				return *this;
			}
			const_iterator operator-(size_t amount) const {
				const_iterator returnValue = const_iterator(elementPointer);
				returnValue.elementPointer -= amount;
				return returnValue;
			}
			const_iterator operator+(size_t amount) const {
				const_iterator returnValue = const_iterator(elementPointer);
				returnValue.elementPointer += amount;
				return returnValue;
			}
			difference_type operator-(const const_iterator& other) const {
				return elementPointer - other.elementPointer;
			}
			difference_type operator+(const const_iterator& other) const {
				return elementPointer + other.elementPointer;
			}

			bool operator==(const const_iterator& other) const { return elementPointer == other.elementPointer; }

			bool operator!=(const const_iterator& other) const { return elementPointer != other.elementPointer; }

			reference operator*() const { return *elementPointer; }
			pointer operator->() { return elementPointer; }
			const pointer operator->() const { return elementPointer; }

		  private:
			pointer elementPointer;
		};

		/**
		 * \brief Adds an element to the slotmap.
		 *
		 * \param newElement The element that should be added to the slotmap.
		 * \returns The handle of the new element.
		 */
		inline SlotmapHandle addElement(const T& newElement);

		/**
		 * \brief Adds an element to the slotmap.
		 *
		 * \param newElement The element that should be added to the slotmap.
		 * \returns The handle of the new element.
		 */
		inline SlotmapHandle addElement(T&& newElement);

		/**
		 * \brief Gets the element that belongs to the specified handle.
		 *
		 * \param handle The specified handle.
		 * \returns The element belonging to the specified handle. If "handle" is invalid, an assert will trigger
		 */
		inline T& elementAt(SlotmapHandle handle);

		/**
		 * \brief Gets the element that belongs to the specified handle.
		 *
		 * \param handle The specified handle.
		 * \returns The element belonging to the specified handle. If "handle" is invalid, an assert will trigger
		 */
		inline const T& elementAt(SlotmapHandle handle) const;

		/**
		 * \brief Removes the element specified by its handle.
		 *
		 * Removes the element specified by its handle. If the handle is invalid, nothing happens.
		 *
		 * \param handle The handle of the element to remove.
		 */
		inline void removeElement(SlotmapHandle handle);

		inline T& operator[](SlotmapHandle handle);

		inline const T& operator[](SlotmapHandle handle) const;

		/**
		 * \brief Wipes all elements of the slotmap.
		 */
		inline void clear();

		/**
		 * \brief Gets the number of elements in the slotmap.
		 *
		 * \returns The number of elements in the slotmap. 0 if there are none.
		 */
		inline size_t size() const;

		/**
		 * \returns An iterator of the begin of the elements.
		 */
		inline iterator begin();

		/**
		 * \returns An iterator of the end of the elements.
		 */
		inline iterator end();

		/**
		 * \returns An iterator of the begin of the elements.
		 */
		inline const_iterator cbegin() const;

		/**
		 * \returns An iterator of the end of the elements.
		 */
		inline const_iterator cend() const;

		/**
		 * \returns An iterator of the element with the specified handle.
		 */
		inline iterator find(SlotmapHandle handle);

		/**
		 * \returns An iterator of the element with the specified handle.
		 */
		inline const_iterator find(SlotmapHandle handle) const;

		/**
		 * \returns The handle of the specified iterator.
		 */
		inline SlotmapHandle handle(const iterator& handleIterator);

	  private:
		// All elements.
		std::vector<T> elements;
		std::vector<size_t> keys;
		std::vector<size_t> eraseMap;
		size_t freeKeyHead = 0;
		size_t freeKeyTail = 0;
	};

	template <typename T> inline SlotmapHandle Slotmap<T>::addElement(const T& newElement) {
		if (keys.size() == 0)
			keys.push_back(0);
		elements.push_back(newElement);
		eraseMap.push_back(freeKeyHead);
		if (freeKeyHead == freeKeyTail) {
			size_t newFreeSlotIndex = keys.size();

			keys.push_back(newFreeSlotIndex);

			keys[freeKeyTail] = newFreeSlotIndex;
			freeKeyTail = newFreeSlotIndex;
		}
		size_t nextFreeIndex = keys[freeKeyHead];

		keys[freeKeyHead] = elements.size() - 1;

		size_t returnIndex = freeKeyHead;
		freeKeyHead = nextFreeIndex;

		return returnIndex;
	}

	template <typename T> inline SlotmapHandle Slotmap<T>::addElement(T&& newElement) {
		if (keys.size() == 0)
			keys.push_back(0);
		elements.push_back(std::forward<T>(newElement));
		eraseMap.push_back(freeKeyHead);
		if (freeKeyHead == freeKeyTail) {
			size_t newFreeSlotIndex = keys.size();

			keys.push_back(newFreeSlotIndex);

			keys[freeKeyTail] = newFreeSlotIndex;
			freeKeyTail = newFreeSlotIndex;
		}
		size_t nextFreeIndex = keys[freeKeyHead];

		keys[freeKeyHead] = elements.size() - 1;

		size_t returnIndex = freeKeyHead;
		freeKeyHead = nextFreeIndex;

		return returnIndex;
	}

	template <typename T> inline T& Slotmap<T>::elementAt(SlotmapHandle handle) {
		assert(keys.size() > handle);
		assert(eraseMap[keys[handle]] == handle);
		return elements[keys[handle]];
	}

	template <typename T> inline const T& Slotmap<T>::elementAt(SlotmapHandle handle) const {
		assert(keys.size() > handle);
		assert(eraseMap[keys[handle]] == handle);
		return elements[keys[handle]];
	}

	template <typename T> inline void Slotmap<T>::removeElement(SlotmapHandle handle) {
		assert(keys.size() > handle);

		size_t eraseElementIndex = keys[handle];

		// Move last element in, update erase table
		elements[eraseElementIndex] = std::move(elements[elements.size() - 1]);
		eraseMap[eraseElementIndex] = eraseMap[elements.size() - 1];

		// Update key index of what was the last element
		keys[eraseMap[eraseElementIndex]] = eraseElementIndex;

		// Update erase table/element std::vector sizes
		elements.erase(elements.begin() + (elements.size() - 1));
		eraseMap.erase(eraseMap.begin() + (eraseMap.size() - 1));

		// Update free list nodes
		keys[freeKeyTail] = handle;
		keys[handle] = handle;
		freeKeyTail = handle;
	}

	template <typename T> inline T& Slotmap<T>::operator[](SlotmapHandle handle) { return elementAt(handle); }

	template <typename T> inline const T& Slotmap<T>::operator[](SlotmapHandle handle) const {
		return elementAt(handle);
	}

	template <typename T> inline void Slotmap<T>::clear() {
		elements.clear();
		keys.clear();
		eraseMap.clear();
		freeKeyHead = 0;
		freeKeyTail = 0;
	}

	template <typename T> inline size_t Slotmap<T>::size() const { return elements.size(); }

	template <typename T> inline typename Slotmap<T>::iterator Slotmap<T>::begin() { return iterator(elements.data()); }

	template <typename T> inline typename Slotmap<T>::iterator Slotmap<T>::end() {
		return iterator(elements.data() + elements.size());
	}

	template <typename T> inline typename Slotmap<T>::const_iterator Slotmap<T>::cbegin() const {
		return const_iterator(elements.data());
	}

	template <typename T> inline typename Slotmap<T>::const_iterator Slotmap<T>::cend() const {
		return const_iterator(elements.data() + elements.size());
	}

	template <typename T> inline typename Slotmap<T>::iterator Slotmap<T>::find(SlotmapHandle handle) {
		if (handle >= keys.size())
			return iterator(elements.data() + elements.size());
		return iterator(elements.data() + keys[handle]);
	}

	template <typename T> inline typename Slotmap<T>::const_iterator Slotmap<T>::find(SlotmapHandle handle) const {
		if (handle >= keys.size())
			return const_iterator(elements.data() + elements.size());
		return const_iterator(elements.data() + keys[handle]);
	}

	template <typename T> inline SlotmapHandle Slotmap<T>::handle(const Slotmap<T>::iterator& handleIterator) {
		return { eraseMap[static_cast<T*>(handleIterator) - elements.data()] };
	}

} // namespace vanadium