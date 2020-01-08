
#include <chrono>
#include <vector>
#include <map>
#include <string>
#include <exception>
#include <fstream>

#include <filesystem>

#include "Subsampler.h"
#include "Subsampler_PoissonDisc.h"
#include "Metadata.h"
#include "LASLoader.hpp"
#include "Chunker.h"
#include "Vector3.h"
#include "ChunkProcessor.h"
#include "PotreeWriter.h"

using namespace std::experimental;

namespace fs = std::experimental::filesystem;

void saveChunk(string path, ChunkerCell& cell, int i) {
	if (cell.count == 0) {
		return;
	}

	auto file = std::fstream(path, std::ios::out | std::ios::binary);

	for (Points* batch : cell.batches) {

		vector<Vector3<double>> coordinates;
		for (Point& point : batch->points) {
			coordinates.emplace_back(point.x, point.y, point.z);
		}

		const char* data = reinterpret_cast<const char*>(coordinates.data());
		int64_t size = coordinates.size() * sizeof(Vector3<double>);

		file.write(data, size);

	}

	for (Points* batch : cell.batches) {
		const char* data = reinterpret_cast<const char*>(batch->attributeBuffer->dataU8);
		int64_t size = batch->attributeBuffer->size;

		file.write(data, size);
	}

	file.close();
}

int gridSizeFromPointCount(uint64_t pointCount) {
	if (pointCount < 10'000'000) {
		return 2;
	} if (pointCount < 100'000'000) {
		return 4;
	} else if (pointCount < 1'000'000'000) {
		return 8;
	} else if (pointCount < 10'000'000'000) {
		return 16;
	} else if (pointCount < 100'000'000'000) {
		return 32;
	} else{
		return 64;
	}
}


future<Chunker*> chunking(LASLoader* loader, Metadata metadata) {

	//Chunker* chunker = new Chunker(metadata.targetDirectory, metadata.chunkGridSize);


	string path = "D:/temp/test/chunks";
	for (const auto& entry : fs::directory_iterator(path)){
		fs::remove(entry);
	}

	Vector3<double> size = metadata.max - metadata.min;
	double cubeSize = std::max(std::max(size.x, size.y), size.z);
	Vector3<double> cubeMin = metadata.min;
	Vector3<double> cubeMax = cubeMin + cubeSize;

	//chunker->min = cubeMin;
	//chunker->max = cubeMax;
	Chunker* chunker = new Chunker(path, cubeMin, cubeMax, 2);

	int batchNumber = 0;
	Points* batch = co_await loader->nextBatch();
	while (batch != nullptr) {
		if ((batchNumber % 10) == 0) {
			cout << "batch loaded: " << batchNumber << endl;
		}

		chunker->add(batch);

		batch = co_await loader->nextBatch();

		batchNumber++;
	}

	chunker->close();

	return chunker;
}


future<void> run() {

	//string path = "D:/dev/pointclouds/Riegl/Retz_Airborne_Terrestrial_Combined_1cm.las";
	//string path = "D:/dev/pointclouds/Riegl/niederweiden.las";
	string path = "D:/dev/pointclouds/archpro/heidentor.las";
	//string path = "D:/dev/pointclouds/mschuetz/lion.las";
	//string path = "D:/dev/pointclouds/Riegl/Retz_Airborne_Terrestrial_Combined_1cm.las";
	//string path = "D:/dev/pointclouds/open_topography/ca13/morro_rock/merged.las";
	//string targetDirectory = "C:/temp/test";
	string targetDirectory = "C:/dev/workspaces/potree/develop/test/new_format";

	auto tStart = now();

	LASLoader* loader = new LASLoader(path);

	auto size = loader->max - loader->min;
	double octreeSize = size.max();


	Metadata metadata;
	metadata.targetDirectory = targetDirectory;
	metadata.min = loader->min;
	metadata.max = loader->min + octreeSize;
	metadata.numPoints = loader->numPoints;
	//metadata.chunkGridSize = gridSizeFromPointCount(metadata.numPoints);

	int upperLevels = 3;
	metadata.chunkGridSize = pow(2, upperLevels);

	Chunker* chunker = co_await chunking(loader, metadata);

	//vector<Chunk*> chunks = getListOfChunks(metadata);

	//double scale = 0.001;
	//double spacing = 1.0;
	//PotreeWriter writer(targetDirectory, 
	//	metadata.min,
	//	metadata.max,
	//	spacing,
	//	scale,
	//	upperLevels
	//);

	//vector<thread> threads;
	//for (Chunk* chunk : chunks) {
	////for (Chunk* chunk : {chunks[0], chunks[1]}) {

	//	threads.emplace_back(thread([&writer, chunk](){
	//		loadChunk(chunk);
	//		Node* chunkRoot = processChunk(chunk);

	//		writer.writeChunk(chunk, chunkRoot);
	//	}));
	//}

	//for (thread& t : threads) {
	//	t.join();
	//}

	//writer.close();

	


	auto tEnd = now();
	auto duration = tEnd - tStart;
	cout << "duration: " << duration << endl;

	co_return;
}

int main(int argc, char **argv){

	//cout << sizeof(Point) << endl;

	run().wait();

	return 0;
}

