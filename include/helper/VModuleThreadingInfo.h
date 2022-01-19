#pragma once

#include <VModule.hpp>
#include <atomic>
#include <optional>

// unsafe (not atomic) to copy or move, but this is never done by VEngine
class AtomicBoolWrapper {
  public:
	AtomicBoolWrapper(bool val) : value(val) {}
	AtomicBoolWrapper(const AtomicBoolWrapper& other) { value.store(other.value.load()); }
	AtomicBoolWrapper& operator=(const AtomicBoolWrapper& other) {
		value.store(other.value.load());
		return *this;
	}
	AtomicBoolWrapper(AtomicBoolWrapper&& other) { value.store(other.value.load()); }
	AtomicBoolWrapper& operator=(AtomicBoolWrapper&& other) {
		value.store(other.value.load());
		return *this;
	}

	std::atomic<bool> value;
};

struct VModuleDependency {
	VModule* from;
	VModule* to;
};

struct VModuleExecutionInfo {
	std::vector<std::atomic<bool>*> waitAtomics;
	VModule* executedModule;
	std::optional<AtomicBoolWrapper> signalAtomic;
};