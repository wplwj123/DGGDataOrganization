// proj headers
#include "EQRasterReader.h"

// sln headers
#include "../LibEQCommon/EQType.h"
#include "../LibEQUtil/DigitUtil.h"
#include "../LibEQRaster/IEQRaster.h"

// cpp headers
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <math.h>

#ifdef _DEBUG
#pragma comment(lib,"../build/LibEQRasterd.lib")
#else
#pragma comment(lib,"../build/LibEQRaster.lib")
#endif

namespace eq = eqtm;
using std::ifstream;
using std::stringstream;
using std::vector;

void readHeader(ifstream& is,
				eq::DataType& dt, eq::ElementType& et, unsigned int& level, unsigned int& nband, float& nodata) {
	printf("-----------FILE'S HEADER------------\n");
	std::string line_str;
	std::string format, data_type, element_type;

	std::string name;
	std::getline(is, line_str);
	stringstream ss(line_str);
	ss >> name >> format;
	printf("%s %s\n", name.c_str(), format.c_str());
	ss.clear();

	std::getline(is, line_str);
	ss.str(line_str);
	ss >> name >> data_type;
	printf("%s %s\n", name.c_str(), data_type.c_str());
	if (data_type == "raster") {
		dt = eq::DataType::RASTER;
	}
	ss.clear();

	std::getline(is, line_str);
	ss.str(line_str);
	ss >> name >> element_type;
	printf("%s %s\n", name.c_str(), element_type.c_str());
	if (element_type == "cell")
		et = eq::ElementType::EQ_CELL;
	else if (element_type == "edge")
		et = eq::ElementType::EQ_EDGE;
	else if (element_type == "node")
		et = eqtm::EQ_NODE;
	else {
		printf("unkonwn element type!");
		assert(0);
	}
	ss.clear();

	std::getline(is, line_str);
	ss.str(line_str);
	ss >> name >> level;
	printf("%s %d\n", name.c_str(), level);
	ss.clear();

	std::getline(is, line_str);
	ss.str(line_str);
	ss >> name >> nband;
	printf("%s %d\n", name.c_str(), nband);
	ss.clear();

	std::getline(is, line_str);
	ss.str(line_str);
	ss >> name >> nodata;
	printf("%s %f\n", name.c_str(), nodata);
	ss.clear();

	std::getline(is, line_str);
	printf("------------------------------------\n");
}

void readMbs(ifstream& is, const unsigned int& level,
			 vector<eq::EQCode>& offsetlst, vector<EQ_ULLONG>& sizelst) {
	printf("-------------FILE'S MBS-------------\n");
	std::string line_str;
	stringstream ss;

	for (std::getline(is, line_str); line_str != "end_mbs"; std::getline(is, line_str)) {
		std::string code_str;
		unsigned long long size = 0;

		auto pos = line_str.find(',');
		code_str = line_str.substr(0, pos);
		eq::EQCode code;
		auto domain = eq::util::binary_to_decimal(
			reinterpret_cast<const unsigned char*>(code_str.substr(0, 4).c_str()));
		auto type = eq::util::binary_to_decimal(
			reinterpret_cast<const unsigned char*>(code_str.substr(4, 4).c_str()));
		code.dt = (domain << 4) + type;
		code.len = level;

		//Converts the quaternary to decimal
		EQ_ULLONG temp = atoll(code_str.substr(8).c_str());
		EQ_ULLONG morton = 0;
		EQ_UINT power = 0;

		while (temp != 0){
			morton = morton + (temp % 10) * pow(4, power);
			temp = temp / 10;
			power++;
		}

		code.morton = morton;
		offsetlst.push_back(code);

		printf("%llu,%llu\n", domain, code.morton);

		size = atoi(line_str.substr(pos + 1).c_str());
		sizelst.push_back(size);
		ss.clear();
	}
	printf("------------------------------------\n");
}

eqtm::IEQRaster* EQRasterReader::readFile(const std::string& filename) {
	ifstream is;
	is.open(filename, std::ios::in);

	if (!is.is_open()) {
		printf("fail to open file: %s\n", filename.c_str());
		assert(0);
	}

	std::string line_str;
	std::getline(is, line_str);

	if (line_str != "eqtm_header") {
		printf("incompatible file format\n");
		assert(0);
	}

	// read header
	eq::DataType dt;
	eq::ElementType et;
	unsigned int level = 0, nband = 0;
	float nodata;
	readHeader(is, dt, et, level, nband, nodata);

	eq::IEQRaster* ir = eq::CreateRasterObject();
	ir->setHeader(dt, et, level, nband, nodata);

	// read mbs
	vector<eq::EQCode> offset_lst;
	vector<unsigned long long> size_lst;
	readMbs(is, level, offset_lst, size_lst);

	for (size_t i = 0; i != offset_lst.size(); ++i) {
		ir->setMBS(0, offset_lst[i], size_lst[i]);
	}

	// read body
	char punct;
	is >> punct;

	unsigned int ele_doom = 0, first_ele = 0; // doom is 1 bigger than last element's bin
	switch (et) {
	case eqtm::EQ_NODE:
		ele_doom = 1;
		first_ele = 0x00; // 0000
		break;
	case eqtm::EQ_EDGE:
		ele_doom = 4;
		first_ele = 0x01; // 0001	
		break;
	case eqtm::EQ_CELL:
		ele_doom = 6;
		first_ele = 0x04; // 0100	
		break;
	default: break;
	}

	for (size_t i = 0; i < offset_lst.size(); ++i) {
		eq::EQCode code = offset_lst[i];
		size_t j = 0;
		while (j < size_lst[i]) {
			if ((code.dt & 0x0f) < ele_doom) { // 0x0f = 15 = 00001111
				is >> punct;
				float attr;
				is >> attr;
#ifdef GRIDCHECK
				if (attr != nodata){
					eqtm::DestroyRasterObject(ir);
					return nullptr;
				}
#endif
				ir->setAttribute(0, code, attr);
				code.dt += 1;
				++j;
			}
			else {
				code.dt = (code.dt & 240) + first_ele; // 240 = 11110000
				code.morton += 1;
			}
		}
		is >> punct >> punct;
	}

	is.close();
	return ir;
}

void EQRasterReader::destroy(eqtm::IEQRaster* er) {
	if (er) {
		eq::DestroyRasterObject(er);
	}
}
