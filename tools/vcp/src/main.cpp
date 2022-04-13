#include <fstream>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#include <filesystem>

#include <ParsingUtils.hpp>
#include <PipelineArchetypeRecord.hpp>
#include <PipelineInstanceRecord.hpp>
#include <SysUtils.hpp>

#include <unordered_map>

#include <helper/WholeFileReader.hpp>

struct Options {
	std::string outFile;
	std::vector<std::string> fileNames;
	std::string compilerCommand = "glslc";
	std::vector<std::string> additionalCommandArgs;
};

bool checkOption(int argc, char** argv, size_t index, const std::string_view& argName) {
	if (argc <= index + 1) {
		std::cout << "Warning: Not enough arguments given to " << argName << ".\n";
		return false;
	} else if (*argv[index + 1] == '-') {
		std::cout << "Warning: " << argv[index + 1] << "Invalid argument for " << argName << ".\n";
		return false;
	}
	return true;
}

size_t parseOption(int argc, char** argv, size_t index, Options& options) {
	if (argv[index] == std::string_view("--compiler-command")) {
		if (checkOption(argc, argv, index, "--compiler-command")) {
			options.compilerCommand = argv[index + 1];
			return index + 1;
		}
	} else if (argv[index] == std::string_view("--compiler-args")) {
		if (checkOption(argc, argv, index, "--compiler-args")) {
			options.additionalCommandArgs = splitString(std::string(argv[index + 1]), ' ', false);
			return index + 1;
		}
	} else if (argv[index] == std::string_view("-o")) {
		if (checkOption(argc, argv, index, "-o")) {
			options.outFile = argv[index + 1];
			return index + 1;
		}
	} else {
		std::cout << "Warning: Skipping unknown command line option " << argv[index] << ".\n ";
	}
	return index;
}

Options parseArguments(int argc, char** argv) {
	Options options;
	for (size_t i = 1; i < argc; ++i) {
		if (*argv[i] == '-') {
			i = parseOption(argc, argv, i, options);
			continue;
		}
		options.fileNames.push_back(argv[i]);
	}
	return options;
}

struct PipelineRecord {
	PipelineArchetypeRecord archetypeRecord;
	std::vector<PipelineInstanceRecord> instanceRecords;
	uint64_t totalRecordSize;
	uint64_t instanceOffset;
};

using namespace std::filesystem;

int main(int argc, char** argv) {
	Options options = parseArguments(argc, argv);

	if (options.outFile.empty()) {
		std::cout << "Error: No output file specified." << std::endl;
		return EXIT_FAILURE;
	}

	path tempDirPath = path("./temp");
	std::error_code error;
	if (!create_directory(tempDirPath, error)) {
		std::cout << "Error: Could not create temporary shader output directory inside the project directory."
				  << std::endl;
		return EXIT_FAILURE;
	}

	std::ofstream outStream = std::ofstream(options.outFile, std::ios::trunc);
	if (!outStream.is_open()) {
		std::cout << "Error: Output file could not be opened for writing." << std::endl;
		remove_all(tempDirPath);
		return EXIT_FAILURE;
	}

	uint32_t version = 4;
	outStream.write(reinterpret_cast<char*>(&version), sizeof(uint32_t));
	uint32_t pipelineCount = static_cast<uint32_t>(options.fileNames.size());
	outStream.write(reinterpret_cast<char*>(&pipelineCount), sizeof(uint32_t));

	auto records = std::vector<PipelineRecord>();
	std::vector<path> localRecordPaths;
	std::vector<std::vector<DescriptorBindingLayoutInfo>> setLayoutInfos;

	records.reserve(options.fileNames.size());
	localRecordPaths.reserve(options.fileNames.size());

	size_t recordIndex = 0;
	for (auto& name : options.fileNames) {
		path localRecordPath = tempDirPath;
		if (!create_directory(localRecordPath.append(std::to_string(recordIndex)))) {
			std::cout << "Error: Could not create temporary shader output directory inside the project directory."
					  << std::endl;
			remove_all(tempDirPath);
			return EXIT_FAILURE;
		}

		path filePath = absolute(name);
		size_t fileSize;
		char* data = reinterpret_cast<char*>(readFile(name.c_str(), &fileSize));
		if (!data) {
			std::cout << "Error: Pipeline file not found in the working directory." << std::endl;
			outStream.close();
			remove(options.outFile);
			remove_all(tempDirPath);
			return EXIT_FAILURE;
		}

		Json::Value rootValue;

		Json::Reader reader;
		reader.parse(data, data + fileSize, rootValue);

		delete[] data;

		if (!rootValue["archetype"].isObject()) {
			std::cout << "Error: JSON archetype is invalid." << std::endl;
			outStream.close();
			remove(options.outFile);
			remove_all(tempDirPath);
			return EXIT_FAILURE;
		}
		if (!rootValue["instances"].isArray()) {
			std::cout << "Error: JSON instance array is invalid." << std::endl;
			outStream.close();
			remove(options.outFile);
			remove_all(tempDirPath);
			return EXIT_FAILURE;
		}

		records.push_back(
			{ .archetypeRecord = PipelineArchetypeRecord(filePath.string(), filePath.parent_path().c_str(),
														 setLayoutInfos, rootValue["archetype"]) });
		if (!records[recordIndex].archetypeRecord.isValid()) {
			outStream.close();
			remove(options.outFile);
			remove_all(tempDirPath);
			return EXIT_FAILURE;
		}

		records[recordIndex].archetypeRecord.compileShaders(localRecordPath.string(), options.compilerCommand,
															options.additionalCommandArgs);

		std::vector<PipelineInstanceRecord> instanceRecords;
		instanceRecords.reserve(rootValue["instances"].size());
		for (auto& instance : rootValue["instances"]) {
			instanceRecords.push_back(
				PipelineInstanceRecord(records[recordIndex].archetypeRecord.type(), filePath.string(), instance));
		}
		records[recordIndex].instanceRecords = std::move(instanceRecords);
		localRecordPaths.push_back(localRecordPath);
		++recordIndex;
	}

	uint32_t setCount = static_cast<uint32_t>(setLayoutInfos.size());
	outStream.write(reinterpret_cast<char*>(&setCount), sizeof(uint32_t));
	uint64_t currentFileOffset = 3 * sizeof(uint32_t) + setLayoutInfos.size() * sizeof(uint64_t);
	for (auto& set : setLayoutInfos) {
		outStream.write(reinterpret_cast<char*>(&currentFileOffset), sizeof(uint64_t));
		uint32_t bindingCount = static_cast<uint32_t>(set.size());
		uint64_t bindingSize = 0;
		for (auto& binding : set) {
			uint64_t samplerInfoSize = 8ULL * sizeof(uint32_t) + 4ULL * sizeof(float) + 3ULL * sizeof(bool);
			bindingSize += 5 * sizeof(uint32_t) + sizeof(bool) + binding.immutableSamplerInfos.size() * samplerInfoSize;
		}
		currentFileOffset += sizeof(uint32_t) + bindingSize;
	}

	for (auto& set : setLayoutInfos) {
		uint32_t bindingCount = static_cast<uint32_t>(set.size());
		outStream.write(reinterpret_cast<char*>(&bindingCount), sizeof(uint32_t));

		for (auto& binding : set) {
			outStream.write(reinterpret_cast<char*>(&binding.binding.binding), sizeof(uint32_t));
			outStream.write(reinterpret_cast<char*>(&binding.binding.descriptorType), sizeof(uint32_t));
			outStream.write(reinterpret_cast<char*>(&binding.binding.descriptorCount), sizeof(uint32_t));
			outStream.write(reinterpret_cast<char*>(&binding.binding.stageFlags), sizeof(uint32_t));
			outStream.write(reinterpret_cast<char*>(&binding.usesImmutableSamplers), sizeof(bool));
			uint32_t immutableSamplerCount = binding.immutableSamplerInfos.size();
			outStream.write(reinterpret_cast<char*>(&immutableSamplerCount), sizeof(uint32_t));

			for (auto& sampler : binding.immutableSamplerInfos) {
				outStream.write(reinterpret_cast<char*>(&sampler.magFilter), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.minFilter), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.mipmapMode), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.addressModeU), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.addressModeV), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.addressModeW), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.mipLodBias), sizeof(float));
				outStream.write(reinterpret_cast<char*>(&sampler.anisotropyEnable), sizeof(bool));
				outStream.write(reinterpret_cast<char*>(&sampler.maxAnisotropy), sizeof(float));
				outStream.write(reinterpret_cast<char*>(&sampler.compareEnable), sizeof(bool));
				outStream.write(reinterpret_cast<char*>(&sampler.compareOp), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.minLod), sizeof(float));
				outStream.write(reinterpret_cast<char*>(&sampler.maxLod), sizeof(float));
				outStream.write(reinterpret_cast<char*>(&sampler.borderColor), sizeof(uint32_t));
				outStream.write(reinterpret_cast<char*>(&sampler.unnormalizedCoordinates), sizeof(bool));
			}
		}
	}

	uint32_t fileNameIndex = 0;
	currentFileOffset += sizeof(uint64_t) * options.fileNames.size();
	for (auto& record : records) {
		path filePath = absolute(options.fileNames[fileNameIndex]);
		auto shaders = record.archetypeRecord.retrieveCompileResults(filePath.string(), localRecordPaths[fileNameIndex].string());

		outStream.write(reinterpret_cast<char*>(&currentFileOffset), sizeof(uint64_t));
		record.totalRecordSize = 0;
		// number of instances/offsets of instances
		record.totalRecordSize += sizeof(uint32_t);
		record.totalRecordSize += record.instanceRecords.size() * sizeof(uint32_t);

		record.totalRecordSize += record.archetypeRecord.serializedSize();

		record.instanceOffset = currentFileOffset + record.totalRecordSize;

		for (auto& instance : record.instanceRecords) {
			record.totalRecordSize += instance.serializedSize();
		}
		currentFileOffset += record.totalRecordSize;
		record.archetypeRecord.verifyArchetype(filePath.string(), setLayoutInfos, shaders);

		bool isValid = record.archetypeRecord.isValid();

		for (auto& instance : record.instanceRecords) {
			instance.verifyInstance(filePath.string(), shaders);
			isValid &= instance.isValid();
		}
		if (!isValid) {
			outStream.close();
			remove(options.outFile);
			remove_all(tempDirPath);
			return EXIT_FAILURE;
		}
		++fileNameIndex;
	}

	for (auto& record : records) {
		void* dstData = new char[record.totalRecordSize];
		void* nextData = dstData;

		record.archetypeRecord.serialize(nextData);
		nextData = offsetVoidPtr(nextData, record.archetypeRecord.serializedSize());

		uint32_t instanceCount = record.instanceRecords.size();
		std::memcpy(nextData, &instanceCount, sizeof(uint32_t));
		nextData = offsetVoidPtr(nextData, sizeof(uint32_t));
		for (auto& instance : record.instanceRecords) {
			std::memcpy(nextData, &record.instanceOffset, sizeof(uint32_t));
			nextData = offsetVoidPtr(nextData, sizeof(uint32_t));
			record.instanceOffset += instance.serializedSize();
		}

		for (auto& instance : record.instanceRecords) {
			instance.serialize(nextData);
			nextData = offsetVoidPtr(nextData, instance.serializedSize());
		}

		outStream.write(reinterpret_cast<char*>(dstData), record.totalRecordSize);

		currentFileOffset += record.totalRecordSize;

		delete[] reinterpret_cast<char*>(dstData);
	}

	remove_all(tempDirPath);

	return 0;
}