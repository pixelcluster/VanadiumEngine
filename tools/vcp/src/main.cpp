#include <fstream>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>
#include <ctime>

#include <filesystem>

#include <ParsingUtils.hpp>
#include <PipelineArchetypeRecord.hpp>
#include <PipelineInstanceRecord.hpp>
#include <SysUtils.hpp>

#include <unordered_map>

#include <util/WholeFileReader.hpp>

struct Options {
	std::string outFile;
	std::vector<std::string> fileNames;
	std::string compilerCommand = "glslc";
	std::vector<std::string> additionalCommandArgs;
};

bool checkOption(int argc, char** argv, size_t index, const std::string_view& argName) {
	if (static_cast<size_t>(argc) <= index + 1) {
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
	for (size_t i = 1; i < static_cast<size_t>(argc); ++i) {
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
};

using namespace std::filesystem;

int main(int argc, char** argv) {
	Options options = parseArguments(argc, argv);

	if (options.outFile.empty()) {
		std::cout << "Error: No output file specified." << std::endl;
		return EXIT_FAILURE;
	}

	std::srand(std::clock());
	path tempDirPath = path("./vcptemp-" + std::to_string(std::rand()));
	std::error_code error;
	if (exists(tempDirPath)) {
		remove_all(tempDirPath);
	}
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
		vanadium::logInfo("new archetype");

		std::string parentPathString = filePath.parent_path().string();
		records.push_back(
			{ .archetypeRecord = PipelineArchetypeRecord(filePath.string(), parentPathString.c_str(),
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

	uint32_t fileNameIndex = 0;
	for (auto& record : records) {
		path filePath = absolute(options.fileNames[fileNameIndex]);
		auto shaders =
			record.archetypeRecord.retrieveCompileResults(filePath.string(), localRecordPaths[fileNameIndex].string());

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
			for (auto& shader : shaders) {
				spvReflectDestroyShaderModule(&shader.shader);
			}
			return EXIT_FAILURE;
		}
		for (auto& shader : shaders) {
			spvReflectDestroyShaderModule(&shader.shader);
		}
		++fileNameIndex;
	}

	// Construct VCP file

	serialize(VCPFileHeader{}, outStream);
	serializeVector(setLayoutInfos, outStream);

	for (auto& record : records) {
		record.archetypeRecord.serialize(outStream);
		serializeVector(record.instanceRecords, outStream);

		record.archetypeRecord.freeShaders();
	}

	remove_all(tempDirPath);

	return 0;
}