// project headers
#include "EQType.h"
#include "EQCodec.h"

// sln heades
#include "../LibEQUtil/TypeUtil.h"
#include "../LibEQUtil/DigitUtil.h"
#include "../LibEQUtil//GeoUtil.h"
#include "../LibEQUtil/CodeUtil.h"

// c++ headers
#include <cassert>
#include <cmath>
#include <string>

#ifdef _DEBUG
#pragma comment(lib,"../build/LibEQUtild.lib")
#else
#pragma comment(lib,"../build/LibEQUtil.lib")
#endif

namespace eqtm {
	EQCode encode(const SphericCoord& sc, const EQ_UINT& level, const ElementType& ele) {
#ifdef _DEBUG
		assert(abs(sc.latitude) <= 90.f && abs(sc.longitude) <= 180.0);
#endif

		EQCode c;
		c.len = level;
		c.morton = 0;

		// check pole
		if (ele == EQ_NODE) { // if node
			if (abs(sc.latitude - 90.0) <= EQ_EPS) { // north pole
				c.dt = 0xa0;
				c.morton = 0;
				return c;
			}

			if (abs(sc.latitude + 90.0) <= EQ_EPS) { // south pole
				c.dt = 0xb0;
				c.morton = 0;
				return c;
			}
		}

		// compute which domain
		CartesianCoord cc = util::spheric_to_cartesian(sc);
		unsigned int domain = util::predict_domain(cc);
		CartesianCoord v0, v1, v2, v3;
		util::get_domain_corner(domain, v0, v1, v2, v3);

		// get morton
		Diamond d(v0, v1, v2, v3);
		EQ_ULLONG temp = 0;
		util::get_morton(temp, cc, d, level); // bitwise operation
		c.morton = temp;

		// get domain and type
		c.dt = util::get_dt(cc, d, ele);
		return c;
	}

	SphericCoord decode(const EQCode& code, EQ_UINT& domain, EQ_UINT& level, ElementType& ele) {
		// grid's level
		level = code.len;

		// check pole
		// compute domain
		domain = code.dt >> 4;
		switch (domain) {
			case 10: // north pole
				ele = EQ_NODE;
				return SphericCoord(0, 90.);
			case 11:
				ele = EQ_NODE;
				return SphericCoord(0, -90.);
			default:
				break;
		}

#ifdef _DEBUG
		assert(code.morton < util::eq_pow((EQ_ULLONG)4, code.len));
#endif

		CartesianCoord v0, v1, v2, v3;
		util::get_domain_corner(domain, v0, v1, v2, v3);

		// final sub diamond
		Diamond d(v0, v1, v2, v3);
		util::get_diamond(d, code.morton, level); // bitwise operation

		// get lat/lon
		int type = code.dt & 0x0f;
		CartesianCoord cc = util::get_diamond_element(d, code.dt, ele);
		return util::cartesian_to_spheric(cc);
	}

	Trigon decode(const EQCode& code, unsigned& domain, unsigned& level) {
		// grid's level
		level = code.len;

		// compute domain
		domain = code.dt >> 4;

#ifdef _DEBUG
		assert(code.morton < util::eq_pow((EQ_ULLONG)4, code.len)); // let it crash!
#endif

		CartesianCoord v0, v1, v2, v3;
		util::get_domain_corner(domain, v0, v1, v2, v3);

		// final sub diamond
		Diamond d(v0, v1, v2, v3);
		util::get_diamond(d, code.morton, level); // bitwise operation

		Trigon t;
		if (code.dt & 15 - 4) {
			t.v(0) = util::cartesian_to_spheric(d.c2);
			t.v(1) = util::cartesian_to_spheric(d.c1);
			t.v(2) = util::cartesian_to_spheric(d.c3);
		}
		else {
			t.v(0) = util::cartesian_to_spheric(d.c0);
			t.v(1) = util::cartesian_to_spheric(d.c3);
			t.v(2) = util::cartesian_to_spheric(d.c1);
		}
		return t;
	}

	void getSubCells(const eqtm::EQCode& cell,
	                 eqtm::EQCode& cell0,
	                 eqtm::EQCode& cell1,
	                 eqtm::EQCode& cell2,
	                 eqtm::EQCode& cell3) {
		if ((cell.dt & 0x0f) - 0x04) { // under cell
			eqtm::EQCode code;
			code.dt = (cell.dt & 0xf0) + 0x05;
			code.morton = (cell.morton << 2) + 0x00;
			code.len = cell.len + 1;
			cell0 = code;

			code.dt = (cell.dt & 0xf0) + 0x04;
			code.morton = (cell.morton << 2) + 0x02;
			code.len = cell.len + 1;
			cell1 = code;

			code.dt = (cell.dt & 0xf0) + 0x05;
			code.morton = (cell.morton << 2) + 0x02;
			code.len = cell.len + 1;
			cell2 = code;

			code.dt = (cell.dt & 0xf0) + 0x05;
			code.morton = (cell.morton << 2) + 0x03;
			code.len = cell.len + 1;
			cell3 = code;
		}
		else {
			eqtm::EQCode code;
			code.dt = (cell.dt & 0xf0) + 0x04;
			code.morton = (cell.morton << 2) + 0x00;
			code.len = cell.len + 1;
			cell0 = code;

			code.dt = (cell.dt & 0xf0) + 0x04;
			code.morton = (cell.morton << 2) + 0x01;
			code.len = cell.len + 1;
			cell1 = code;

			code.dt = (cell.dt & 0xf0) + 0x05;
			code.morton = (cell.morton << 2) + 0x01;
			code.len = cell.len + 1;
			cell2 = code;

			code.dt = (cell.dt & 0xf0) + 0x04;
			code.morton = (cell.morton << 2) + 0x03;
			code.len = cell.len + 1;
			cell3 = code;
		}
	}

	EQCode getParentCell(const eqtm::EQCode& cell) {
		eqtm::EQCode ret;
		ret.len = cell.len - 1;
		ret.morton = cell.morton >> 2;

		if ((cell.morton & 0x03) == 0x01) { // 0x01 sub diamond
			ret.dt = (cell.dt & 0xf0) + 0x04;
		}

		else if ((cell.morton & 0x03) == 0x02) { // 0x02 sub diamond
			ret.dt = (cell.dt & 0xf0) + 0x05;
		}
		else if ((cell.dt & 0x0f) - 0x04) { // downward
			ret.dt = (cell.dt & 0xf0) + 0x05;
		}
		else {
			ret.dt = (cell.dt & 0xf0) + 0x04;
		}

		return ret;
	}
}
