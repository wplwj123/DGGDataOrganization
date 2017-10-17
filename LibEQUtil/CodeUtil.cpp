// project headers
#include "TypeUtil.h"
#include "CodeUtil.h"
#include "GeoUtil.h"
#include "DigitUtil.h"

// sln heades
#include "../LibEQCommon/EQType.h"

// c++ headers
#include <cassert>
#include <vector>
#include <algorithm>

EQ_UCHAR eqtm::util::get_dt(const CartesianCoord& cc, const Diamond& d, const ElementType& ele) {
	// domain + type
	EQ_UCHAR dt = 0;
	EQ_UINT domain = util::predict_domain(cc);

	switch (ele) {
	case EQ_NODE:
		// use bit op
		dt = (domain << 4) + 0;
		break;

	case EQ_EDGE:
		dt = get_nearest_edge(cc, d, domain);
		break;

	case EQ_CELL:
		dt = get_located_cell(cc, d, domain);
		break;
	}

	return dt;
}

EQ_UINT eqtm::util::get_domian(const EQCode& code) {
	// return code.dt / 16;
	// use bit op
	return  code.dt >> 4;
}

eqtm::ElementType eqtm::util::get_type(const EQCode& code) {
	// int rem = code.dt % 16;
	int rem = code.dt & 15;

	switch (rem) {
	case 0:
		return EQ_NODE;
	case 1: case 2: case 3:
		return EQ_EDGE;
	case 4: case 5:
		return EQ_CELL;
	default:
		return EQ_NODE;
	}
}

void eqtm::util::get_morton(EQ_UCHAR* morton,
	const CartesianCoord& cc, Diamond& d, const EQ_UINT& level, EQ_UINT count) {
	if (level <= 0) {
		morton[count] = '\0';
		return;
	}
#if 0
	CartesianCoord M0, M1, M2, M3;
	M0 = mid_great_arc(d.c0, d.c1);
	M1 = mid_great_arc(d.c1, d.c2);
	M2 = mid_great_arc(d.c2, d.c3);
	M3 = mid_great_arc(d.c3, d.c0);
#endif

	d = get_sub_diamond(d, cc, morton[count++]);
	get_morton(morton, cc, d, level - 1, count);
}

void eqtm::util::get_morton(EQ_ULLONG& morton,
	const CartesianCoord& cc, Diamond& d, const EQ_UINT& level, EQ_UINT count) {
	if (level <= 0) {
		return;
	}

	EQ_UCHAR subid;
	d = get_sub_diamond(d, cc, subid);
	morton = morton << 2;
	morton += subid - '0';
	get_morton(morton, cc, d, level - 1, count);
}

void eqtm::util::get_diamond(Diamond& d,
	EQ_UCHAR* morton, const EQ_UINT& level, EQ_UINT count) {
	if (level <= 0) {
		return;
	}

	d = get_sub_diamond(d, morton[count++]);
	get_diamond(d, morton, level - 1, count);
}

void eqtm::util::get_diamond(Diamond& d,
	const EQ_ULLONG& morton, const unsigned& level, unsigned count) {
	if (level <= 0) {
		return;
	}

	EQ_ULLONG base = 2;
	EQ_ULLONG and_value = util::eq_pow(base, level * 2 - 1)
		+ util::eq_pow(base, level * 2 - 2);

	EQ_ULLONG ret = morton & and_value;
	ret = ret >> (level * 2 - 2);
	char subid = '0' + static_cast<unsigned>(ret);

	d = get_sub_diamond(d, subid);
	get_diamond(d, morton, level - 1, count);
}

eqtm::Diamond eqtm::util::get_sub_diamond(const Diamond& d, const EQ_UCHAR& subid) {
	CartesianCoord M0, M1, M2, M3;
	diamond_mids(d, M0, M1, M2, M3);
	
	CartesianCoord center = diamond_center(d);
#if 0
	center = intersection(M0, M2, M1, M3);
#endif

	int temp = subid - '0';
	Diamond sub_diamond{ CartesianCoord{ 0., 0., 0. },
		CartesianCoord{ 0., 0., 0. }, 
		CartesianCoord{ 0., 0., 0. }, 
		CartesianCoord{ 0., 0., 0. }};
	if (temp == 1) {
		sub_diamond.c0 = d.c0;
		sub_diamond.c1 = M0;
		sub_diamond.c2 = center;
		sub_diamond.c3 = M3;
	}

	else if (temp == 3) {
		sub_diamond.c0 = M0;
		sub_diamond.c1 = d.c1;
		sub_diamond.c2 = M1;
		sub_diamond.c3 = center;
	}

	else if (temp == 2) {
		sub_diamond.c0 = center;
		sub_diamond.c1 = M1;
		sub_diamond.c2 = d.c2;
		sub_diamond.c3 = M2;
	}

	else {
		sub_diamond.c0 = M3;
		sub_diamond.c1 = center;
		sub_diamond.c2 = M2;
		sub_diamond.c3 = d.c3;
	}

	return sub_diamond;
}

eqtm::Diamond eqtm::util::get_sub_diamond(const Diamond& d, const CartesianCoord& cc, EQ_UCHAR& subid) {
	CartesianCoord M0, M1, M2, M3;
	diamond_mids(d, M0, M1, M2, M3);

	CartesianCoord center = diamond_center(d);

#if 0
	EQ_UCHAR subid_map[2][2] = { '2', '0', '3', '1' };
	int pos0 = point_above_plane(cc, M3, M1);
	int pos1 = point_above_plane(cc, M2, M0);
	subid = subid_map[pos0][pos1];
#else
	// sacrifice some efficiency for tidy code
	int pos0 = point_above_plane(cc, center, M0);
	int pos1 = point_above_plane(cc, center, M1);
	int pos2 = point_above_plane(cc, center, M2);
	int pos3 = point_above_plane(cc, center, M3);

	if (pos3 == 1 && pos2 == -1) {
		subid = '0';
	}
	else if (pos0 == 1 && pos3 != 1) {
		subid = '1';
	}
	else if (pos1 == -1 && pos2 != -1) {
		subid = '2';
	}
	else {
		subid = '3';
	}
#endif
	return get_sub_diamond(d, subid);
}

eqtm::CartesianCoord eqtm::util::get_diamond_element(const Diamond& d, const EQ_UCHAR& dt, ElementType& ele) {
	// int type = dt % 16;
	int type = dt & 15;

	CartesianCoord cc;
	switch (type) {
	case 0:
		ele = EQ_NODE;
		cc = d.c3;
		break;
	case 1:
		ele = EQ_EDGE;
		cc = mid_great_arc(d.c3, d.c0);
		break;
	case 2:
		ele = EQ_EDGE;
		cc = mid_great_arc(d.c3, d.c1);
		break;
	case 3:
		ele = EQ_EDGE;
		cc = mid_great_arc(d.c3, d.c2);
		break;
	case 4:
		ele = EQ_CELL;
		cc = (d.c0 + d.c1 + d.c3) * (1.f / 3.f);
		cc = normalize(cc, EQ_RADIUS);
		break;
	case 5:
		ele = EQ_CELL;
		cc = (d.c2 + d.c1 + d.c3) * (1.f / 3.f);
		cc = normalize(cc, EQ_RADIUS);
		break;
	default:
		break;
	}
	return cc;
}

EQ_UCHAR eqtm::util::get_nearest_edge(const CartesianCoord& cc, const Diamond& d, const EQ_UINT& domain) {
	eqtm::CartesianCoord edge_nw = mid_great_arc(d.c0, d.c3); // northwest
	eqtm::CartesianCoord edge_c = mid_great_arc(d.c1, d.c3); // center
	eqtm::CartesianCoord edge_sw = mid_great_arc(d.c2, d.c3); // southwest

	std::vector<double> dist(3);
	dist[0] = linear_length(edge_nw, cc);
	dist[1] = linear_length(edge_c, cc);
	dist[2] = linear_length(edge_sw, cc);

	// compare these values and return nearest edge
	// of course you can compute by urself but for fewer lines of code...
	std::vector<double>::iterator cur = std::min_element(dist.begin(), dist.end());
	int pos = static_cast<int>(std::distance(dist.begin(), cur));

	dist.clear();
	dist.shrink_to_fit();

	// use bit op
	EQ_UCHAR dt = (domain << 4) + pos + 1;
	return dt;
}

EQ_UCHAR eqtm::util::get_located_cell(const CartesianCoord& cc, const Diamond& d, const unsigned& domain) {
	int position = point_above_plane(cc, d.c3, d.c1);
	EQ_UCHAR dt;
	if (position == 1) {
		// use bit op
		dt = (domain << 4) + 4;
	}
	else {
		dt = (domain << 4) + 5;
	}

	return dt;
}