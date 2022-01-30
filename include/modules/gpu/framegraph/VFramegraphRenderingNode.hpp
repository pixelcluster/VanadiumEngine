#pragma once

#include <modules/gpu/framegraph/VFramegraphContext.hpp>

class VFramegraphNode {
  public:
	virtual void setupResources(VFramegraphContext* context) = 0;

	virtual void recordCommands(VFramegraphContext* context) = 0;

	virtual void handleWindowResize(VFramegraphContext* context) = 0;
  private:

};