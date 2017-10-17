// prj headers
#include "Raster.h"
#include "RasterReader.h"

#include "EQCellRenderer.h"
#include "EQEdgeRenderer.h"
#include "EQNodeRenderer.h"

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

namespace eq = eqtm;
namespace equ = eqtm::util;
namespace tcl = TCLAP;

using std::stringstream;
using std::string;
using std::ofstream;

static EQ_UINT g_domain_count = 10;

// get grid level coresponding to raster's cellsize
EQ_UINT raster_level(const CRaster* raster) {
	eq::Icosahedron ico;
	double arc_len = equ::arc_length(ico.p0, ico.p2);

	double unit = (2 * EQ_PI * EQ_RADIUS / 360.0) * static_cast<double>(raster->getCellSize());
	return static_cast<EQ_UINT>(log(arc_len / unit) / log(2.0));
}

eq::IEQRaster* discrete_on_cell(const eq::EQCode areaCode, const CRaster* raster, const EQ_UINT& remlevel = 6){
	eq::IEQRaster* ir = eq::CreateRasterObject();
	ir->setHeader(eq::RASTER, eq::EQ_CELL,
		areaCode.len + remlevel,
		raster->getNBand(),
		raster->getNoDataValue());

	eq::EQCode offset;
	offset.dt = areaCode.dt;
	offset.len = areaCode.len + remlevel;
	offset.morton = (areaCode.morton << (2 * remlevel));

	EQ_ULLONG n_cell_in_domain = (1 << remlevel) * (1 << remlevel);
	ir->setMBS(0, offset, n_cell_in_domain * 2);

	for (size_t mor_id = 0; mor_id < n_cell_in_domain; ++mor_id) {
		eq::EQCode code;
		code.dt = areaCode.dt;
		code.len = areaCode.len + remlevel;
		code.morton = (areaCode.morton << (2 * remlevel)) + mor_id;

		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;
		eq::ElementType code_eid;

		// convert to lat/lon
		eq::SphericCoord sc = eq::decode(code, code_domain, code_level, code_eid);
		// get value
		float value =
			raster->getValueFromBicubicInterpolatation(
			static_cast<float>(sc.longitude),
			static_cast<float>(sc.latitude));
		ir->setAttribute(0, code, value);

		code.dt += 1;
		sc = eq::decode(code, code_domain, code_level, code_eid);
		// get value
		value =
			raster->getValueFromBicubicInterpolatation(
			static_cast<float>(sc.longitude),
			static_cast<float>(sc.latitude));
		ir->setAttribute(0, code, value);
		code.dt -= 1;
	}

	return ir;
}

eq::IEQRaster* discrete_on_edge(const eq::EQCode areaCode, const CRaster* raster, const EQ_UINT& remlevel = 6){
	eq::IEQRaster* ir = eq::CreateRasterObject();
	ir->setHeader(eq::RASTER, eq::EQ_EDGE,
		areaCode.len + remlevel,
		raster->getNBand(),
		raster->getNoDataValue());

	eq::EQCode offset;
	offset.dt = areaCode.dt;
	offset.len = areaCode.len + remlevel;
	offset.morton = (areaCode.morton << (2 * remlevel));

	EQ_ULLONG n_cell_in_domain = (1 << remlevel) * (1 << remlevel);
	ir->setMBS(0, offset, n_cell_in_domain * 3);

	for (size_t mor_id = 0; mor_id < n_cell_in_domain; ++mor_id) {
		eq::EQCode code;
		code.dt = areaCode.dt;
		code.len = areaCode.len + remlevel;
		code.morton = (areaCode.morton << (2 * remlevel)) + mor_id;

		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;
		eq::ElementType code_eid;

		// convert to lat/lon
		eq::SphericCoord sc = eq::decode(code, code_domain, code_level, code_eid);
		// get value
		float value =
			raster->getValueFromBicubicInterpolatation(
			static_cast<float>(sc.longitude),
			static_cast<float>(sc.latitude));
		ir->setAttribute(0, code, value);

		code.dt += 1;
		sc = eq::decode(code, code_domain, code_level, code_eid);
		// get value
		value =
			raster->getValueFromBicubicInterpolatation(
			static_cast<float>(sc.longitude),
			static_cast<float>(sc.latitude));
		ir->setAttribute(0, code, value);

		code.dt += 1;
		sc = eq::decode(code, code_domain, code_level, code_eid);
		// get value
		value =
			raster->getValueFromBicubicInterpolatation(
			static_cast<float>(sc.longitude),
			static_cast<float>(sc.latitude));
		ir->setAttribute(0, code, value);

		code.dt -= 2;
	}

	return ir;
}

eq::IEQRaster* discrete_on_node(const eq::EQCode areaCode, const CRaster* raster, const EQ_UINT& remlevel = 6){
	eq::IEQRaster* ir = eq::CreateRasterObject();
	ir->setHeader(eq::RASTER, eq::EQ_NODE,
		areaCode.len + remlevel,
		raster->getNBand(),
		raster->getNoDataValue());

	eq::EQCode offset;
	offset.dt = areaCode.dt;
	offset.len = areaCode.len + remlevel;
	offset.morton = (areaCode.morton << (2 * remlevel));

	EQ_ULLONG n_cell_in_domain = (1 << remlevel) * (1 << remlevel);
	ir->setMBS(0, offset, n_cell_in_domain);

	for (size_t mor_id = 0; mor_id < n_cell_in_domain; ++mor_id) {
		eq::EQCode code;
		code.dt = areaCode.dt;
		code.len = areaCode.len + remlevel;
		code.morton = (areaCode.morton << (2 * remlevel)) + mor_id;

		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;
		eq::ElementType code_eid;

		// convert to lat/lon
		eq::SphericCoord sc = eq::decode(code, code_domain, code_level, code_eid);
		// get value
		float value =
			raster->getValueFromBicubicInterpolatation(
			static_cast<float>(sc.longitude),
			static_cast<float>(sc.latitude));
		ir->setAttribute(0, code, value);
	}

	return ir;
}

void write_cell(eqtm::IEQRaster* er, const std::string& fileName) {
	// get maximum and minimum
	float max = 9000;
	float min = -11000.;

	// push nodes and colors into renderer
	IRasterRenderer* irenderer = new EQCellRenderer();
	std::map<eq::EQCode, EQ_UINT> code_idx_mapper;
	unsigned int node_idx = 0;

	for (unsigned int i = 0; i < er->getMBSCount(); ++i) {
		eq::EQCode offset;
		EQ_ULLONG size;
		er->getMBS(0, i, offset, size);

		if (size == 0) {
			continue;
		}

		eq::EQCode code = offset;
		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;

		unsigned int index = 0;
		while (index < size / 2) {
			code.dt = (code.dt & 240); // &11110000, last 4 bit reset to 0000
			//eq::SphericCoord sc = eq::decode(code, code_domain, code_level, code_eid);
			//irenderer->addNode(sc);

			code.dt = code.dt + 0x04; // upper cell
			eq::Trigon tri = eq::decode(code, code_domain, code_level);
			eq::EQCode code_v0 = eq::encode(tri.v(0), code_level, eq::EQ_NODE);
			eq::EQCode code_v1 = eq::encode(tri.v(1), code_level, eq::EQ_NODE);
			eq::EQCode code_v2 = eq::encode(tri.v(2), code_level, eq::EQ_NODE);

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
			// north pole
			//if ((code_v0.dt >> 4) >= 10) {
			//	v0 = size / 2;
			//}
			//else {
			//	v0 = code_idx_mapper[code_v0];
			//}
			v0 = code_idx_mapper[code_v0];
			v1 = code_idx_mapper[code_v1];
			v2 = code_idx_mapper[code_v2];

			static_cast<EQCellRenderer*>(irenderer)->addIndices(v0, v1, v2);
			float value = er->getAttribute(0, code);
			irenderer->addAttr((value - min) / (max - min));

			code.dt = code.dt + 0x01; // under cell
			tri = eq::decode(code, code_domain, code_level);
			code_v0 = eq::encode(tri.v(0), code_level, eq::EQ_NODE);
			code_v1 = eq::encode(tri.v(1), code_level, eq::EQ_NODE);
			code_v2 = eq::encode(tri.v(2), code_level, eq::EQ_NODE);

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

			// south pole
			//if ((code_v0.dt >> 4) >= 10) {
			//	v0 = size / 2 + 1;
			//}
			//else {
			//	v0 = code_idx_mapper[code_v0];
			//}
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
	irenderer->addNode(eq::SphericCoord(0., 90.));
	irenderer->addNode(eq::SphericCoord(0., -90.));
	irenderer->render();

	osgDB::writeNodeFile(*irenderer->getNode(), fileName);

	delete irenderer;

}

void write_edge(eqtm::IEQRaster* er, const std::string& fileName) {
	float max = 9000;
	float min = -11000.;

	// push nodes and colors into renderer
	IRasterRenderer* irenderer = new EQEdgeRenderer;
	std::map<eq::EQCode, EQ_UINT> code_idx_mapper;
	unsigned int node_idx = 0;

	for (unsigned int i = 0; i < er->getMBSCount(); ++i) {
		eq::EQCode offset = {};
		EQ_ULLONG size = 0;
		er->getMBS(0, i, offset, size);

		if (size == 0) {
			continue;
		}

		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;

		eq::EQCode code = offset;
		for (size_t mor_id = 0; mor_id < size / 3; ++mor_id) {
			code.morton = mor_id;
			code.dt = (code.dt & 240); // &11110000, last 4 bit reset to 0000
			//eq::SphericCoord sc = eq::decode(code, code_domain, code_level, code_eid);
			//irenderer->addNode(sc);

			code.dt = code.dt + 0x04; // upper cell
			eq::Trigon tri = eq::decode(code, code_domain, code_level);
			eq::EQCode code_v0 = eq::encode(tri.v(0), code_level, eq::EQ_NODE);
			eq::EQCode code_v3 = eq::encode(tri.v(1), code_level, eq::EQ_NODE);
			eq::EQCode code_v1 = eq::encode(tri.v(2), code_level, eq::EQ_NODE);

			if (code_idx_mapper.find(code_v0) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(0));
				code_idx_mapper.insert(std::make_pair(code_v0, node_idx));
				node_idx++;
			}
			if (code_idx_mapper.find(code_v3) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(1));
				code_idx_mapper.insert(std::make_pair(code_v3, node_idx));
				node_idx++;
			}
			if (code_idx_mapper.find(code_v1) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(2));
				code_idx_mapper.insert(std::make_pair(code_v1, node_idx));
				node_idx++;
			}

			code.dt = code.dt + 0x01; // under cell
			tri = eq::decode(code, code_domain, code_level);
			eq::EQCode code_v2 = eq::encode(tri.v(0), code_level, eq::EQ_NODE);

			if (code_idx_mapper.find(code_v2) == code_idx_mapper.end()){
				irenderer->addNode(tri.v(0));
				code_idx_mapper.insert(std::make_pair(code_v2, node_idx));
				node_idx++;
			}

			int v0 = 0, v1 = 0, v2 = 0, v3 = 0;
			// north pole
			if ((code_v0.dt >> 4) >= 10) {
				v0 = size / 3;
			}
			else {
				v0 = code_idx_mapper[code_v0];
			}

			// south pole
			if ((code_v2.dt >> 4) >= 10) {
				v2 = size / 3 + 1;
			}
			else {
				v2 = code_idx_mapper[code_v2];
			}

			v1 = code_idx_mapper[code_v1];
			v3 = code_idx_mapper[code_v3];

			static_cast<EQEdgeRenderer*>(irenderer)->addIndices(v3, v0);
			static_cast<EQEdgeRenderer*>(irenderer)->addIndices(v3, v1);
			static_cast<EQEdgeRenderer*>(irenderer)->addIndices(v3, v2);

			code.dt &= 241; // &11110001
			float value = er->getAttribute(0, code);
			irenderer->addAttr((value - min) / (max - min));
			code.dt += 1;
			value = er->getAttribute(0, code);
			irenderer->addAttr((value - min) / (max - min));
			code.dt += 1;
			value = er->getAttribute(0, code);
			irenderer->addAttr((value - min) / (max - min));
		}
	}

	// poles
	irenderer->addNode(eq::SphericCoord(0., 90.));
	irenderer->addNode(eq::SphericCoord(0., -90.));
	irenderer->render();

	osgDB::writeNodeFile(*irenderer->getNode(), fileName);

	delete irenderer;
}

void write_node(eqtm::IEQRaster* er, const std::string& fileName) {
	float max = 9000;
	float min = -11000.;

	// push nodes and colors into renderer
	IRasterRenderer* irenderer = new EQNodeRenderer;
	for (unsigned int i = 0; i < er->getMBSCount(); ++i) {
		eq::EQCode offset;
		EQ_ULLONG size = 0;
		er->getMBS(0, i, offset, size);
		if (size == 0) {
			continue;
		}
		eq::EQCode code = offset;

		EQ_UINT code_domain = 0;
		EQ_UINT code_level = 0;
		eq::ElementType code_eid;

		for (size_t mor_id = 0; mor_id < size; ++mor_id) {
			code.morton = mor_id;
			code.dt = (code.dt & 240); // &11110000, last 4 bit reset to 0000
			eq::SphericCoord sc = eq::decode(code, code_domain, code_level, code_eid);
			irenderer->addNode(sc);
			float value = er->getAttribute(0, code);
			irenderer->addAttr((value - min) / (max - min));
		}
	}

	// poles
	irenderer->addNode(eq::SphericCoord(0., 90.));
	irenderer->addNode(eq::SphericCoord(0., -90.));

	irenderer->addAttr(0.5f);
	irenderer->addAttr(0.5f);

	irenderer->render();

	osgDB::writeNodeFile(*irenderer->getNode(), fileName);

	delete irenderer;

}

int main(int argc, char** argv) {

	std::string raster_name;
	std::string output_name;
	unsigned int min_level, max_level;
	eq::ElementType ele_type;

	try {
		tcl::CmdLine cmd("Command description message", ' ', "0.1");
		tcl::ValueArg<std::string> raster_arg("r", "raster", "raster file path", true, "homer", "string");
		cmd.add(raster_arg);
		tcl::ValueArg<std::string> output_arg("o", "output", "output file path", true, "homer", "string");
		cmd.add(output_arg);
		tcl::ValueArg<unsigned int> min_level_Arg("i", "minlevel", "min grid level", true, 0, "int");
		cmd.add(min_level_Arg);
		tcl::ValueArg<unsigned int> max_level_Arg("a", "maxlevel", "max grid level", true, 10, "int");
		cmd.add(max_level_Arg);
		tcl::SwitchArg cell_arg("c", "cell", "discrete on cell", false);
		cmd.add(cell_arg);
		tcl::SwitchArg edge_arg("e", "edge", "discrete on egde", false);
		cmd.add(edge_arg);
		tcl::SwitchArg node_arg("n", "node", "discrete on node", false);
		cmd.add(node_arg);
		cmd.parse(argc, argv);

		raster_name = raster_arg.getValue();
		output_name = output_arg.getValue();
		min_level = min_level_Arg.getValue();
		max_level = max_level_Arg.getValue();

		if (min_level > max_level){
			throw tcl::ArgException("minLevel is great than maxLevel!");
		}

		if (cell_arg.getValue())
			ele_type = eqtm::EQ_CELL;
		else if (edge_arg.getValue())
			ele_type = eqtm::EQ_EDGE;
		else
			ele_type = eqtm::EQ_NODE;
	}
	catch (tcl::ArgException& e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		return EXIT_FAILURE;
	}

	// read raster file
	CRaster* ras_ptr = RasterReader::readRaster(raster_name);
	std::shared_ptr<CRaster> ras_sptr(ras_ptr);
	printf("raster's level is %d\n", raster_level(ras_sptr.get()));

	//create Pyramid folder in output path
	if (_mkdir((output_name + "\\Pyramid").c_str()) == 0){
		for (unsigned int level = min_level; level <= max_level; level++){

			std::stringstream ss;
			ss << output_name << "\\Pyramid\\Level-" << level;
			std::string level_file_name = ss.str();

			//create Level folder in Pyramid folder
			if (_mkdir(level_file_name.c_str()) == 0){

				for (size_t dom_id = 0; dom_id < 10; dom_id++){
					eq::EQCode areaCode;

					switch (ele_type) {
					case eqtm::EQ_CELL:
						areaCode.dt = 0x04 + static_cast<int>(dom_id << 4);
						break;
					case eqtm::EQ_EDGE:
						areaCode.dt = 0x01 + static_cast<int>(dom_id << 4);
						break;
					case eqtm::EQ_NODE:
						areaCode.dt = 0x00 + static_cast<int>(dom_id << 4);
						break;
					}

					areaCode.len = level;
					areaCode.morton = 0;

					EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

					//get every tile by morton
					for (size_t mor_id = 0; mor_id < n_cell_in_domain; ++mor_id) {
						areaCode.morton = mor_id;

						eq::IEQRaster* ir = nullptr;

						ss.str("");
						ss.clear();
						ss << level_file_name << "\\" << dom_id << "-" << mor_id << ".ive";
						std::string fileName = ss.str();

						std::cout << level << "-" << dom_id << "-" << mor_id <<  std::endl;

						switch (ele_type) {
						case eqtm::EQ_CELL:
							ir = discrete_on_cell(areaCode, ras_sptr.get(), 6);
							write_cell(ir, fileName);
							break;
						case eqtm::EQ_EDGE:
							ir = discrete_on_edge(areaCode, ras_sptr.get(), 6);
							write_edge(ir, fileName);
							break;
						case eqtm::EQ_NODE:
							ir = discrete_on_node(areaCode, ras_sptr.get(), 6);
							write_node(ir, fileName);
							break;
						}

						eq::DestroyRasterObject(ir);
					}
				}
			}
		}
	}

}