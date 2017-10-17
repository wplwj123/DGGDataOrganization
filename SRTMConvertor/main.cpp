// prj headers
#include "Raster.h"
#include "RasterReader.h"

#include "EQRasterReader.h"
#include "EQCellRenderer.h"

// sln headers
#include "../LibEQUtil/DigitUtil.h"
#include "../LibEQUtil/GeoUtil.h"
#include "../LibEQUtil/TypeUtil.h"

#include "../LibEQCommon/EQType.h"
#include "../LibEQCommon/EQCodec.h"

#include "../LibEQRaster/IEQRaster.h"

// cpp headers
#include <direct.h>
#include <tclap/CmdLine.h>

#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <map>

#include <osgDB/WriteFile>

#ifdef _DEBUG
#pragma comment(lib,"../build/LibEQUtild.lib")
#pragma comment(lib,"../build/LibEQCommond.lib")
#pragma comment(lib,"../build/LibEQRasterd.lib")
#pragma comment(lib,"osgd.lib")
#pragma comment(lib,"osgDBd.lib")
#pragma comment(lib,"osgUtild.lib")
#pragma comment(lib,"osgViewerd.lib")
#pragma comment(lib,"osgEarthd.lib")
#else
#pragma comment(lib,"../build/LibEQUtil.lib")
#pragma comment(lib,"../build/LibEQCommon.lib")
#pragma comment(lib,"../build/LibEQRaster.lib")
#pragma comment(lib,"osg.lib")
#pragma comment(lib,"osgDB.lib")
#pragma comment(lib,"osgUtil.lib")
#pragma comment(lib,"osgViewer.lib")
#pragma comment(lib,"osgEarth.lib")
#endif

struct Header {
	eqtm::DataType datatype;
	eqtm::ElementType eletype;
	unsigned int level;
	unsigned int nband;
	float nodata;
};

struct Rectangle{
	int maxLon, minLon;
	int maxLat, minLat;

	Rectangle(const eqtm::Trigon& tri1, const eqtm::Trigon& tri2){
		maxLon = static_cast<int>(std::max({ tri1.v(0).longitude, tri1.v(1).longitude, tri1.v(2).longitude, tri2.v(0).longitude, tri2.v(1).longitude, tri2.v(2).longitude }));
		minLon = static_cast<int>(std::min({ tri1.v(0).longitude, tri1.v(1).longitude, tri1.v(2).longitude, tri2.v(0).longitude, tri2.v(1).longitude, tri2.v(2).longitude }));

		maxLat = static_cast<int>(std::max({ tri1.v(0).latitude, tri1.v(1).latitude, tri1.v(2).latitude, tri2.v(0).latitude, tri2.v(1).latitude, tri2.v(2).latitude }));
		minLat = static_cast<int>(std::min({ tri1.v(0).latitude, tri1.v(1).latitude, tri1.v(2).latitude, tri2.v(0).latitude, tri2.v(1).latitude, tri2.v(2).latitude }));
	}
};

//get raster name by lat/lon
std::string getRasterName(int lat, int lon){

	std::stringstream ss;
	if (lat < 10){
		ss << "N0" << lat;
	}
	else{
		ss << "N" << lat;
	}

	if (lon < 100){
		ss << "E0" << lon;
	}
	else{
		ss << "E" << lon;
	}
	ss << ".hgt";
	return ss.str();
}

//get raster ptr by SRTM path
CRaster* getRaster(const std::string& rasterPath){

	CRaster* rasterPtr = nullptr;
	std::fstream _file;
	_file.open(rasterPath, std::ios::in);
	if (_file){
		rasterPtr = RasterReader::readRaster(rasterPath);
	}
	_file.close();
	return rasterPtr;
}

eqtm::IEQRaster* DiscreteCell(eqtm::EQCode& areaCode, const EQ_UINT& remlevel, const std::string& srtmPath){

	EQ_UINT tempDomain, tempLevel;
	eqtm::Trigon triUpper = eqtm::decode(areaCode, tempDomain, tempLevel);
	areaCode.dt += 1;
	eqtm::Trigon triLower = eqtm::decode(areaCode, tempDomain, tempLevel);
	areaCode.dt -= 1;

	Rectangle rect(triUpper, triLower);

	std::map<std::string, std::shared_ptr<CRaster>> mapName2Raster;

	std::string rasterName_00 = getRasterName(rect.minLat, rect.minLon);
	std::shared_ptr<CRaster> rasterSptr_00(getRaster(srtmPath + "\\" + rasterName_00));
	mapName2Raster.insert(std::make_pair(rasterName_00, rasterSptr_00));

	if (rect.maxLat != rect.minLat || rect.maxLon != rect.minLon){        //cell intersect with raster edge
		std::string rasterName_01 = getRasterName(rect.minLat, rect.minLon + 1);
		std::shared_ptr<CRaster> rasterSptr_01(getRaster(srtmPath + "\\" + rasterName_01));
		mapName2Raster.insert(std::make_pair(rasterName_01, rasterSptr_01));

		std::string rasterName_10 = getRasterName(rect.minLat + 1, rect.minLon);
		std::shared_ptr<CRaster> rasterSptr_10(getRaster(srtmPath + "\\" + rasterName_10));
		mapName2Raster.insert(std::make_pair(rasterName_10, rasterSptr_10));

		std::string rasterName_11 = getRasterName(rect.minLat + 1, rect.minLon + 1);
		std::shared_ptr<CRaster> rasterSptr_11(getRaster(srtmPath + "\\" + rasterName_11));
		mapName2Raster.insert(std::make_pair(rasterName_11, rasterSptr_11));
	}

	eqtm::IEQRaster* ir = eqtm::CreateRasterObject();
	float noDataValue;
	bool isNull = true;
	for each (auto iter in mapName2Raster){
		if (iter.second.get() != nullptr){
			isNull = false;
			noDataValue = iter.second->getNoDataValue();
			ir->setHeader(eqtm::RASTER, eqtm::EQ_CELL,
				areaCode.len + remlevel,
				iter.second->getNBand(),
				iter.second->getNoDataValue());
			break;
		}
	}

	//SRTM file is no exist
	if (isNull == true){
		eqtm::DestroyRasterObject(ir);
		mapName2Raster.clear();
		return nullptr;
	}

	eqtm::EQCode offset;
	offset.dt = areaCode.dt;
	offset.len = areaCode.len + remlevel;
	offset.morton = (areaCode.morton << (2 * remlevel));

	EQ_ULLONG n_cell_in_domain = (1 << remlevel) * (1 << remlevel);
	ir->setMBS(0, offset, n_cell_in_domain * 2);

	for (size_t morID = 0; morID < n_cell_in_domain; morID++){
		eqtm::EQCode code;
		code.dt = areaCode.dt;
		code.len = areaCode.len + remlevel;
		code.morton = (areaCode.morton << (2 * remlevel)) + morID;

		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;
		eqtm::ElementType code_eid;

		float value = 0.0;
		eqtm::SphericCoord sc = eqtm::decode(code, code_domain, code_level, code_eid);
		std::string rasterName = getRasterName(static_cast<int>(sc.latitude), static_cast<int>(sc.longitude));
		if (mapName2Raster[rasterName].get() == nullptr){
			value = noDataValue;
		}
		else{
			value =
				mapName2Raster[rasterName]->getValueFromBicubicInterpolatation(
				static_cast<float>(sc.longitude),
				static_cast<float>(sc.latitude));
		}
		ir->setAttribute(0, code, value);

		code.dt += 1;
		sc = eqtm::decode(code, code_domain, code_level, code_eid);
		rasterName = getRasterName(static_cast<int>(sc.latitude), static_cast<int>(sc.longitude));
		if (mapName2Raster[rasterName].get() == nullptr){
			value = noDataValue;
		}
		else{
			value =
				mapName2Raster[rasterName]->getValueFromBicubicInterpolatation(
				static_cast<float>(sc.longitude),
				static_cast<float>(sc.latitude));
		}
		ir->setAttribute(0, code, value);
		code.dt -= 1;
	}

	mapName2Raster.clear();
	return ir;
}

void write_ive(eqtm::IEQRaster* er, const std::string& fileName) {
	// get maximum and minimum
	float max = 9000;
	float min = -11000.;

	// push nodes and colors into renderer
	IRasterRenderer* irenderer = new EQCellRenderer();
	std::map<eqtm::EQCode, EQ_UINT> code_idx_mapper;
	unsigned int node_idx = 0;

	for (unsigned int i = 0; i < er->getMBSCount(); ++i) {
		eqtm::EQCode offset;
		EQ_ULLONG size;
		er->getMBS(0, i, offset, size);

		if (size == 0) {
			continue;
		}

		eqtm::EQCode code = offset;
		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;

		unsigned int index = 0;
		while (index < size / 2) {
			code.dt = (code.dt & 240); // &11110000, last 4 bit reset to 0000

			code.dt = code.dt + 0x04; // upper cell
			eqtm::Trigon tri = eqtm::decode(code, code_domain, code_level);
			eqtm::EQCode code_v0 = eqtm::encode(tri.v(0), code_level, eqtm::EQ_NODE);
			eqtm::EQCode code_v1 = eqtm::encode(tri.v(1), code_level, eqtm::EQ_NODE);
			eqtm::EQCode code_v2 = eqtm::encode(tri.v(2), code_level, eqtm::EQ_NODE);

			if (code_idx_mapper.find(code_v0) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(0));
				code_idx_mapper.insert(std::make_pair(code_v0, node_idx));
				node_idx++;
			}
			if (code_idx_mapper.find(code_v1) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(1));
				code_idx_mapper.insert(std::make_pair(code_v1, node_idx));
				node_idx++;
			}
			if (code_idx_mapper.find(code_v2) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(2));
				code_idx_mapper.insert(std::make_pair(code_v2, node_idx));
				node_idx++;
			}

			int v0 = 0, v1 = 0, v2 = 0;
			v0 = code_idx_mapper[code_v0];
			v1 = code_idx_mapper[code_v1];
			v2 = code_idx_mapper[code_v2];

			static_cast<EQCellRenderer*>(irenderer)->addIndices(v0, v1, v2);
			float value = er->getAttribute(0, code);
			irenderer->addAttr((value - min) / (max - min));

			code.dt = code.dt + 0x01; // under cell
			tri = eqtm::decode(code, code_domain, code_level);
			code_v0 = eqtm::encode(tri.v(0), code_level, eqtm::EQ_NODE);
			code_v1 = eqtm::encode(tri.v(1), code_level, eqtm::EQ_NODE);
			code_v2 = eqtm::encode(tri.v(2), code_level, eqtm::EQ_NODE);

			if (code_idx_mapper.find(code_v0) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(0));
				code_idx_mapper.insert(std::make_pair(code_v0, node_idx));
				node_idx++;
			}
			if (code_idx_mapper.find(code_v1) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(1));
				code_idx_mapper.insert(std::make_pair(code_v1, node_idx));
				node_idx++;
			}
			if (code_idx_mapper.find(code_v2) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(2));
				code_idx_mapper.insert(std::make_pair(code_v2, node_idx));
				node_idx++;
			}
			v0 = code_idx_mapper[code_v0];
			v1 = code_idx_mapper[code_v1];
			v2 = code_idx_mapper[code_v2];
			static_cast<EQCellRenderer*>(irenderer)->addIndices(v0, v1, v2);
			value = er->getAttribute(0, code);
			irenderer->addAttr((value - min) / (max - min));
			code.morton++;
			index++;
		}
	}

	// poles
	irenderer->addNode(eqtm::SphericCoord(0., 90.));
	irenderer->addNode(eqtm::SphericCoord(0., -90.));
	irenderer->render();

	osgDB::writeNodeFile(*irenderer->getNode(), fileName);

	delete irenderer;

}

// write EQRaster into local file
void write_dgg(eqtm::IEQRaster* ir, const std::string& filename) {
	std::ofstream os;
	os.open(filename, std::ios::out);

	if (!os.is_open()) {
		printf("can't write file!\n");
		exit(1);
	}

	eqtm::DataType rid;
	eqtm::ElementType eid;
	EQ_UINT level = 0;
	EQ_UINT n_band = 0;
	float nodata = 0.f;
	ir->getHeader(rid, eid, level, n_band, nodata);

	// header part
	os << "eqtm_header" << std::endl;
	os << "format " << "ascii" << std::endl;
	os << "data_type " << "raster" << std::endl;
	os << "element_type ";
	if (eid == eqtm::EQ_CELL)
		os << "cell" << std::endl;
	else if (eid == eqtm::EQ_EDGE)
		os << "edge" << std::endl;
	else
		os << "node" << std::endl;
	os << "level " << level << std::endl;
	os << "band_num " << n_band << std::endl;
	os << "nodata " << nodata << std::endl;
	os << "end_header" << std::endl;

	// minimal bounding seg part
	eqtm::EQCode offset;
	EQ_ULLONG size = 0;
	std::shared_ptr<EQ_UCHAR> dt_bin(new EQ_UCHAR[9]);
	std::shared_ptr<EQ_UCHAR> mor_bin(new EQ_UCHAR[level + 1]);

	for (EQ_UINT dom_id = 0; dom_id < ir->getMBSCount(); ++dom_id) {
		// ReSharper disable once CppEntityNeverUsed
		ir->getMBS(0, dom_id, offset, size);
		eqtm::util::decimal_to_binary(dt_bin.get(), static_cast<EQ_ULLONG>(offset.dt), 8);
		eqtm::util::decimal_to_quaternary(mor_bin.get(), static_cast<EQ_ULLONG>(offset.morton), level);
		os << dt_bin << mor_bin << ',' << size << std::endl;
	}
	os << "end_mbs" << std::endl;

	// attribute part
	os << '(';

	if (eid == eqtm::EQ_CELL) {
		for (EQ_UINT dom_id = 0; dom_id < ir->getMBSCount(); ++dom_id) {
			os << '(';
			ir->getMBS(0, dom_id, offset, size);
			eqtm::EQCode code = offset;

			for (size_t i = 0; i < size / 2; ++i) {
				code.morton = i + offset.morton;
				float upper_attr = ir->getAttribute(0, code);
				code.dt += 1;
				float under_attr = ir->getAttribute(0, code);
				os << upper_attr << ',' << under_attr << ",";
				code.dt -= 1;
			}

			os.seekp(-1, std::ios::end);
			os << ")," << std::endl;
		}
	}

	if (eid == eqtm::EQ_EDGE) {
		for (EQ_UINT dom_id = 0; dom_id < ir->getMBSCount(); ++dom_id) {
			os << '(';
			ir->getMBS(0, dom_id, offset, size);
			eqtm::EQCode code = offset;

			for (size_t i = 0; i < size / 3; ++i) {
				code.morton = i + offset.morton;
				float upper_attr = ir->getAttribute(0, code);
				code.dt += 1;
				float mid_attr = ir->getAttribute(0, code);
				code.dt += 1;
				float under_attr = ir->getAttribute(0, code);
				os << upper_attr << ',' << mid_attr << "," << under_attr << ",";
				code.dt -= 2;
			}

			os.seekp(-1, std::ios::end);
			os << ")," << std::endl;
		}
	}

	if (eid == eqtm::EQ_NODE) {
		for (EQ_UINT dom_id = 0; dom_id < ir->getMBSCount(); ++dom_id) {
			os << '(';
			ir->getMBS(0, dom_id, offset, size);
			eqtm::EQCode code = offset;

			for (size_t i = 0; i < size; ++i) {
				code.morton = i + offset.morton;
				float attr = ir->getAttribute(0, code);
				os << attr << ',';
			}

			os.seekp(-1, std::ios::end);
			os << ")," << std::endl;
		}
	}

	// cr-lf takes 2 bytes
	os.seekp(-3, std::ios::end);
	os << ")";
	os.close();
}

void createBtmPyramid(const std::string& outputPath, const int level, const std::string& srtmPath){

	for (int domID = 1; domID < 2; domID++){
		eqtm::EQCode areaCode;
		areaCode.dt = 0x04 + static_cast<int>(domID << 4);
		areaCode.len = level;
		areaCode.morton = 0;

		EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

		for (size_t morID = 0; morID < n_cell_in_domain; morID++){
			areaCode.morton = morID;

			std::cout << level << "-" << domID << "-" << morID << std::endl;

			eqtm::IEQRaster* ir = nullptr;
			ir = DiscreteCell(areaCode, 6, srtmPath);
			if (ir != nullptr && !ir->isEmpty()){
				std::stringstream ss;
				ss << outputPath << "\\" << domID << "-" << morID << ".dgg";
				std::string dggName = ss.str();
				write_dgg(ir, dggName);

				ss.str("");
				ss.clear();
				ss << outputPath << "\\" << domID << "-" << morID << ".ive";
				std::string iveName = ss.str();
				write_ive(ir, iveName);
				
				eqtm::DestroyRasterObject(ir);
			}
		}
	}
}

eqtm::IEQRaster* getSubEQRaster(const std::string& filePath){

	eqtm::IEQRaster* ir = nullptr;

	bool haveFile = false;
	std::fstream _file;
	_file.open(filePath, std::ios::in);
	if (_file){
		haveFile = true;
	}
	_file.close();

	if (haveFile == true){
		ir = EQRasterReader::readFile(filePath);
	}

	return ir;
}

float getValue(const std::vector<float>& subValue, const float nodataValue, const bool isUpper){

	int* subIndex = new int[4];
	if (isUpper == true){
		subIndex[0] = 0;
		subIndex[1] = 2;
		subIndex[2] = 3;
		subIndex[3] = 6;
	}
	else{
		subIndex[0] = 1;
		subIndex[1] = 4;
		subIndex[2] = 5;
		subIndex[3] = 7;
	}

	float sum = 0;
	int num = 0;
	for (int i = 0; i < 4; i++){
		if (subValue.at(subIndex[i]) != nodataValue){
			sum = sum + subValue.at(subIndex[i]);
			num++;
		}
	}

	if (num == 0){
		delete[] subIndex;
		return nodataValue;
	}

	delete[] subIndex;

	return sum / num;
}

eqtm::IEQRaster* DiscreteCell(const std::string& btmLevelPath, const eqtm::EQCode& areaCode, const EQ_UINT& remlevel){

	std::map<int, eqtm::IEQRaster*> mapIndex2EQRaster;
	for (int i = 0; i < 4; i++){
		std::stringstream ss;
		ss << btmLevelPath << "\\" << (areaCode.dt >> 4) << "-" << (areaCode.morton << 2) + i << ".dgg";
		std::string filePath = ss.str();

		mapIndex2EQRaster.insert(std::make_pair(i, getSubEQRaster(filePath)));
	}

	eqtm::IEQRaster* ir = eqtm::CreateRasterObject();;
	float noDataValue;
	bool isNull = true;
	for each (auto iter in mapIndex2EQRaster){
		if (iter.second != nullptr){
			isNull = false;

			Header h;
			iter.second->getHeader(h.datatype, h.eletype, h.level, h.nband, h.nodata);
			ir->setHeader(h.datatype, h.eletype, h.level - 1, h.nband, h.nodata);
			noDataValue = h.nodata;
			break;
		}
	}

	if (isNull == true){
		eqtm::DestroyRasterObject(ir);
		mapIndex2EQRaster.clear();
		return nullptr;
	}

	eqtm::EQCode offset;
	offset.dt = areaCode.dt;
	offset.len = areaCode.len + remlevel;
	offset.morton = (areaCode.morton << (2 * remlevel));

	EQ_ULLONG n_cell_in_domain = (1 << remlevel) * (1 << remlevel);
	ir->setMBS(0, offset, n_cell_in_domain * 2);

	for (size_t morID = 0; morID < n_cell_in_domain; morID++){
		eqtm::EQCode code;
		code.dt = areaCode.dt;
		code.len = areaCode.len + remlevel;
		code.morton = (areaCode.morton << (2 * remlevel)) + morID;

		int index = static_cast<int>(morID / (n_cell_in_domain / 4));

		if (mapIndex2EQRaster[index] == nullptr){
			ir->setAttribute(0, code, noDataValue);
			code.dt += 1;
			ir->setAttribute(0, code, noDataValue);
			continue;
		}

		std::vector<float> subValue;
		for (int i = 0; i < 4; i++){
			eqtm::EQCode subCode;
			subCode.dt = code.dt;
			subCode.len = code.len + 1;
			subCode.morton = (code.morton << 2) + i;

			subValue.push_back(mapIndex2EQRaster[index]->getAttribute(0, subCode));
			subCode.dt += 1;
			subValue.push_back(mapIndex2EQRaster[index]->getAttribute(0, subCode));
		}

		float upperValue = getValue(subValue, noDataValue, true);
		ir->setAttribute(0, code, upperValue);

		float lowerValue = getValue(subValue, noDataValue, false);
		code.dt += 1;
		ir->setAttribute(0, code, lowerValue);

		subValue.clear();
	}

	for each (auto iter in mapIndex2EQRaster){
		if (iter.second != nullptr){
			eqtm::DestroyRasterObject(iter.second);
		}
	}

	mapIndex2EQRaster.clear();
	return ir;
}

void createPyramid(const std::string& btmLevelPath, const std::string& curLevelPath, const int level){

	for (int domID = 1; domID < 2; domID++){
		eqtm::EQCode areaCode;
		areaCode.dt = 0x04 + static_cast<int>(domID << 4);
		areaCode.len = level;
		areaCode.morton = 0;

		EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

		for (size_t morID = 0; morID < n_cell_in_domain; morID++){
			areaCode.morton = morID;

			std::cout << level << "-" << domID << "-" << morID << std::endl;
			eqtm::IEQRaster* ir = nullptr;

			ir = DiscreteCell(btmLevelPath, areaCode, 6);

			if (ir != nullptr){
				std::stringstream ss;
				ss << curLevelPath << "\\" << domID << "-" << morID << ".dgg";
				std::string dggName = ss.str();
				write_dgg(ir, dggName);

				ss.str("");
				ss.clear();
				ss << curLevelPath << "\\" << domID << "-" << morID << ".ive";
				std::string iveName = ss.str();
				write_ive(ir, iveName);

				eqtm::DestroyRasterObject(ir);
			}
		}
	}
}

int main(int argc, char** argv){

	std::string srtmPath;
	std::string outputPath;
	unsigned int minLevel, maxLevel;

	try {
		TCLAP::CmdLine cmd("Command description message", ' ', "0.1");
		TCLAP::ValueArg<std::string> srtmPathArg("s", "srtm", "srtm file folder path", true, "homer", "string");
		cmd.add(srtmPathArg);
		TCLAP::ValueArg<std::string> outputPathArg("o", "output", "output file path", true, "homer", "string");
		cmd.add(outputPathArg);
		TCLAP::ValueArg<unsigned int> minLevelArg("i", "minlevel", "min grid level", true, 0, "int");
		cmd.add(minLevelArg);
		TCLAP::ValueArg<unsigned int> maxLevelArg("a", "maxlevel", "max grid level", true, 10, "int");
		cmd.add(maxLevelArg);
		cmd.parse(argc, argv);

		srtmPath = srtmPathArg.getValue();
		outputPath = outputPathArg.getValue();
		minLevel = minLevelArg.getValue();
		maxLevel = maxLevelArg.getValue();

		if (minLevel > maxLevel){
			throw TCLAP::ArgException("minLevel is great than maxLevel!");
		}
	}
	catch (TCLAP::ArgException& e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		return EXIT_FAILURE;
	}

	std::stringstream ss;
	ss << outputPath << "\\Level-" << maxLevel;
	std::string maxLevelPath = ss.str();

	if (_mkdir(maxLevelPath.c_str()) == 0){
		createBtmPyramid(maxLevelPath, maxLevel, srtmPath);
	}

	for (unsigned int level = maxLevel - 1; level >= minLevel; level--){
		ss.str("");
		ss.clear();
		ss << outputPath << "\\Level-" << level + 1;
		std::string btmLevelPath = ss.str();

		ss.str("");
		ss.clear();
		ss << outputPath << "\\Level-" << level;
		std::string curLevelPath = ss.str();

		if (_mkdir(curLevelPath.c_str()) == 0){
			createPyramid(btmLevelPath, curLevelPath, level);
		}
	}

	return 0;
}