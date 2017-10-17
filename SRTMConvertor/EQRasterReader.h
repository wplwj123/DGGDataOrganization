#pragma once

#include <string>

namespace eqtm {
	class IEQRaster;
}

class EQRasterReader {
public:
	// note: remember to release eqraster's memory!
	static eqtm::IEQRaster* readFile(const std::string& filename);
	static void destroy(eqtm::IEQRaster* er);
};