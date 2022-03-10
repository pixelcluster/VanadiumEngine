#include <PipelineArchetypeRecord.hpp>

void PipelineArchetypeRecord::addShaderSource(ShaderSourceFile&& file) {
	m_files.push_back(std::forward<ShaderSourceFile>(file));
}

PipelineArchetypeRecord::PipelineArchetypeRecord(const Json::Value& archetypeRoot) {
	
}
