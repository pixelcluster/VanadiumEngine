#pragma once

#include <VEngine.hpp>

class HelloWorldModule : public VModule {
  public:
	void onCreate(VEngine& engine) override;
	void onActivate(VEngine& engine) override;
	void onExecute(VEngine& engine,
				   const std::unordered_map<std::string, const VInputOutputVariable*>& inputVariables) override;
	void onDeactivate(VEngine& engine) override;
	void onDestroy(VEngine& engine) override;

  private:
	size_t m_nExecutions;
};