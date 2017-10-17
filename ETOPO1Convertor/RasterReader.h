#pragma once
#include <string>

class CRaster;

class RasterReader{
public:
	static CRaster* readRaster(const std::string& fileName, const int& band = 0);
private:
	RasterReader() = delete;
	~RasterReader() = delete;
};