#include <graphics/framegraph/FramegraphNode.hpp>
#include <entt/entity/registry.hpp>

namespace vanadium::renderer {

	enum class PassCommand {
		Draw,
		Dispatch
	};

	class Pass {
	  public:
		Pass();
		~Pass();

		virtual void update(entt::registry& renderableRegistry) = 0;

	  private:
		graphics::FramegraphNode* m_framegraphNode;
	};

} // namespace vanadium::renderer
