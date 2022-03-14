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
	std::string fileName;
	std::string projectDir;
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
	if (argv[index] == std::string_view("--working-dir")) {
		if (checkOption(argc, argv, index, "--working-dir")) {
			options.projectDir = argv[index + 1];
			if(options.projectDir.back() != '/')
				options.projectDir.push_back('/');
			return index + 1;
		}
	} else if (argv[index] == std::string_view("--compiler-command")) {
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
		} else if (!options.fileName.empty()) {
			std::cout << "Warning: The input filename was specified more than once. The last name will be chosen.\n";
		}
		options.fileName = argv[i];
	}
	return options;
}

using namespace std::filesystem;

int main(int argc, char** argv) {
	Options options = parseArguments(argc, argv);

	if (options.outFile.empty()) {
		std::cout << "Error: No output file specified." << std::endl;
		return EXIT_FAILURE;
	}
	if (!options.projectDir.empty())
		current_path(options.projectDir);

	path filePath = absolute(options.fileName);

	size_t fileSize;
	char* data = reinterpret_cast<char*>(readFile(options.fileName.c_str(), &fileSize));
	if (!data) {
		std::cout << "Error: Pipeline file not found in the working directory." << std::endl;
		return EXIT_FAILURE;
	}


	Json::Value rootValue;

	Json::Reader reader;
	reader.parse(data, data + fileSize, rootValue);

	free(data);

	if (!rootValue["archetype"].isObject()) {
		std::cout << "Error: JSON archetype is invalid." << std::endl;
		return EXIT_FAILURE;
	}
	if (!rootValue["instances"].isArray()) {
		std::cout << "Error: JSON instance array is invalid." << std::endl;
		return EXIT_FAILURE;
	}

	path tempDirPath = path(options.projectDir + "/temp");
	std::error_code error;
	if (!create_directory(tempDirPath, error)) {
		std::cout << "Error: Could not create temporary shader output directory inside the project directory."
				  << std::endl;
		return EXIT_FAILURE;
	}

	PipelineArchetypeRecord archetypeRecord =
		PipelineArchetypeRecord(filePath.string(), options.projectDir, rootValue["archetype"]);
	if (!archetypeRecord.isValid()) {
		remove_all(tempDirPath);
		return EXIT_FAILURE;
	}
	archetypeRecord.compileShaders(tempDirPath.string(), options.compilerCommand, options.additionalCommandArgs);

	std::vector<PipelineInstanceRecord> instanceRecords;
	instanceRecords.reserve(rootValue["instances"].size());
	for (auto& instance : rootValue["instances"]) {
		instanceRecords.push_back(PipelineInstanceRecord(archetypeRecord.type(), filePath.string(), instance));
	}

	auto shaders = archetypeRecord.retrieveCompileResults(filePath.string(), tempDirPath.string());
	archetypeRecord.verifyArchetype(filePath.string(), shaders);

	bool isValid = archetypeRecord.isValid();

	for (auto& instance : instanceRecords) {
		instance.verifyInstance(filePath.string(), shaders);
		isValid &= instance.isValid();
	}

	if (isValid) {
		std::ofstream outStream = std::ofstream(options.outFile, std::ios::trunc);
		if (!outStream.is_open()) {
			std::cout << "Error: Output file could not be opened for writing." << std::endl;
			remove_all(tempDirPath);
			return EXIT_FAILURE;
		}

		size_t dstFileSize = sizeof(uint32_t);
		// number of instances/offsets of instances
		dstFileSize += sizeof(uint32_t);
		dstFileSize += instanceRecords.size() * sizeof(uint32_t);

		dstFileSize += archetypeRecord.serializedSize();

		size_t instanceOffset = dstFileSize;

		for (auto& instance : instanceRecords) {
			dstFileSize += instance.serializedSize();
		}

		void* dstData = malloc(dstFileSize);
		void* nextData = dstData;
		uint32_t version = 1;
		std::memcpy(nextData, &version, sizeof(uint32_t));
		nextData = offsetVoidPtr(nextData, sizeof(uint32_t));

		archetypeRecord.serialize(nextData);
		nextData = offsetVoidPtr(nextData, archetypeRecord.serializedSize());

		for (auto& instance : instanceRecords) {
			std::memcpy(nextData, &instanceOffset, sizeof(uint32_t));
			nextData = offsetVoidPtr(nextData, sizeof(uint32_t));
			instanceOffset += instance.serializedSize();
		}

		for (auto& instance : instanceRecords) {
			instance.serialize(nextData);
			nextData = offsetVoidPtr(nextData, instance.serializedSize());
		}

		outStream.write(reinterpret_cast<char*>(dstData), dstFileSize);

		free(dstData);
	}

	remove_all(tempDirPath);

	return isValid ? 0 : EXIT_FAILURE;
}