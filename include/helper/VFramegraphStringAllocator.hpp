#pragma once

#include <malloc.h>
#include <new>

template <typename T> struct VFramegraphStringAllocatorBlock {
	size_t maxSize;
	size_t expandSize;
	T* dataBegin;
	T* dataCurrent;
};

// A simple bump-point allocator (or forwarder to operator new)
template <typename T> class VFramegraphStringAllocator {
  public:
	using value_type = T;

	VFramegraphStringAllocator();

	//this is big bad and isn't guaranteed to work because of alignment, but it seems to work MSVC debug STL requires this
	template <typename U>
	VFramegraphStringAllocator(const VFramegraphStringAllocator<U>& other)
		: m_dataBlock(reinterpret_cast<VFramegraphStringAllocatorBlock<T>*>(other.m_dataBlock)),
		  m_forwardToOperatorNew(other.m_forwardToOperatorNew) {}
	VFramegraphStringAllocator(size_t size);

	T* allocate(size_t n);

	void deallocate(T* ptr, size_t n);

	void expand();

	void clear();

	void destroy();

  private:
	template <typename> friend class VFramegraphStringAllocator;

	VFramegraphStringAllocatorBlock<T>* m_dataBlock;

	bool m_forwardToOperatorNew = false;
};
template <typename T> VFramegraphStringAllocator<T>::VFramegraphStringAllocator() : m_forwardToOperatorNew(true) {}

template <typename T> inline VFramegraphStringAllocator<T>::VFramegraphStringAllocator(size_t size) {
	void* data = malloc(size * sizeof(T) + sizeof(VFramegraphStringAllocatorBlock<T>));
	T* dataBegin = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(data) + sizeof(VFramegraphStringAllocatorBlock<T>));
	m_dataBlock = new (data) VFramegraphStringAllocatorBlock<T> {
		.maxSize = size * sizeof(T), 
		.expandSize = size * sizeof(T), 
		.dataBegin = dataBegin, 
		.dataCurrent = dataBegin
	};
}

template <typename T> inline T* VFramegraphStringAllocator<T>::allocate(size_t n) {
	if (m_forwardToOperatorNew) {
		return new T[n];
	} else {
		if (reinterpret_cast<uintptr_t>(m_dataBlock->dataCurrent) -
				reinterpret_cast<uintptr_t>(m_dataBlock->dataBegin) + n * sizeof(T) <=
			m_dataBlock->maxSize) {
			T* result = m_dataBlock->dataCurrent;
			m_dataBlock->dataCurrent += n;
			return result;
		}
		return new T[n];
	}
}

template <typename T> inline void VFramegraphStringAllocator<T>::deallocate(T* ptr, size_t n) {
	if (m_forwardToOperatorNew ||
		reinterpret_cast<uintptr_t>(ptr) > reinterpret_cast<uintptr_t>(m_dataBlock->dataBegin) + m_dataBlock->maxSize ||
		reinterpret_cast<uintptr_t>(ptr) < reinterpret_cast<uintptr_t>(m_dataBlock->dataBegin)) {
		delete[] ptr;
	} else {
		if (m_dataBlock->dataCurrent == ptr) {
			m_dataBlock->dataCurrent += n;
		}
	}
}

template <typename T> inline void VFramegraphStringAllocator<T>::expand() { m_dataBlock->expandSize *= 2; }

template <typename T> inline void VFramegraphStringAllocator<T>::clear() {
	if (!m_forwardToOperatorNew) {
		if (m_dataBlock->expandSize != m_dataBlock->maxSize) {
			m_dataBlock->maxSize = m_dataBlock->expandSize;
			m_dataBlock = reinterpret_cast<VFramegraphStringAllocatorBlock<T>*>(
				realloc(m_dataBlock, m_dataBlock->maxSize + sizeof(VFramegraphStringAllocatorBlock<T>)));
		}
		m_dataBlock->dataCurrent = m_dataBlock->dataBegin;
	}
}

template <typename T> inline void VFramegraphStringAllocator<T>::destroy() {
	if (!m_forwardToOperatorNew) {
		free(m_dataBlock);
	}
}
