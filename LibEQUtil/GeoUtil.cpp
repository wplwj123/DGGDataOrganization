// project headers
#include "TypeUtil.h"
#include "GeoUtil.h"

// solution headers
#include "../LibEQCommon/EQType.h"

#ifdef USE_PRECISION
#include "../Precision/fprecision.h"
#endif

// c++ headers
#include <cmath>
#include <cassert>

namespace eqtm { namespace util {
	CartesianCoord spheric_to_cartesian(const SphericCoord& sc) {
		SphericCoord tempsc = sc;

		if (tempsc.longitude < 0.) {
			tempsc.longitude = 360 + sc.longitude;
		}

#ifdef USE_PRECISION
			float_precision lat(tempsc.latitude, 200);
			float_precision lon(tempsc.longitude, 200);
			float_precision alpha(lat*float_precision(EQ_PI, 200) / float_precision(180., 200));
			float_precision beta(lon*float_precision(EQ_PI, 200) / float_precision(180., 200));
			double x = float_precision(EQ_RADIUS,200) * cos(alpha) * cos(beta);
			double y = float_precision(EQ_RADIUS,200) * cos(alpha) * sin(beta);
			double z = float_precision(EQ_RADIUS,200) * sin(alpha);
#else
		double x = EQ_RADIUS * cos(tempsc.latitude * EQ_PI / 180.0) * cos(tempsc.longitude * EQ_PI / 180.0);
		double y = EQ_RADIUS * cos(tempsc.latitude * EQ_PI / 180.0) * sin(tempsc.longitude * EQ_PI / 180.0);
		double z = EQ_RADIUS * sin(tempsc.latitude * EQ_PI / 180.0);
#endif
		CartesianCoord cc(x, y, z);
		return cc;
	}

	SphericCoord cartesian_to_spheric(const CartesianCoord& cc) {
		SphericCoord sc;

		if (abs(cc.x - 0) <= EQ_EPS && cc.y > 0) {
			sc.longitude = 90.;
		}
		else if (abs(cc.x - 0) < EQ_EPS && cc.y < 0) {
			sc.longitude = -90.;
		}
		else if (cc.x > 0 && abs(cc.y - 0) < EQ_EPS) {
			sc.longitude = 0.;
		}
		else if (cc.x < 0 && abs(cc.y - 0) < EQ_EPS) {
			sc.longitude = 180.;
		}
		else if (abs(cc.x - 0) < EQ_EPS && abs(cc.y - 0) < EQ_EPS) {
			sc.longitude = 0.;
		}
		else {
			sc.longitude = atan(cc.y / cc.x) / EQ_PI * 180.;
		}

		if (cc.x < 0. && cc.y > 0.) {
			sc.longitude += 180.;
		}
		else if (cc.x < 0. && cc.y < 0.) {
			sc.longitude -= 180.;
		}

		if (sc.longitude < -180.) {
			sc.longitude += 180.;
		}
		else if (sc.longitude > 180.) {
			sc.longitude -= 180.;
		}

#ifdef USE_PRECISION
			float_precision f(cc.z / EQ_RADIUS, 200);
			sc.latitude = asin(f) / float_precision(EQ_PI, 200) * float_precision(180., 200);
#else
		sc.latitude = asin(cc.z / EQ_RADIUS) / EQ_PI * 180.;
#endif
		return sc;
	}

	double inner_angle(const CartesianCoord& arm0, const CartesianCoord& arm1, const CartesianCoord& vertex) {
		double inner_product = (arm0.x - vertex.x) * (arm1.x - vertex.x) + (arm0.y - vertex.y) * (arm1.y - vertex.y) +
				(arm0.z - vertex.z) * (arm1.z - vertex.z);
		double modulus = sqrt((arm0.x - vertex.x) * (arm0.x - vertex.x) + (arm0.y - vertex.y) * (arm0.y - vertex.y) + (arm0.z - vertex.z) * (arm0.z - vertex.z)) *
				sqrt(((arm1.x - vertex.x) * (arm1.x - vertex.x) + (arm1.y - vertex.y) * (arm1.y - vertex.y) + (arm1.z - vertex.z) * (arm1.z - vertex.z)));

		return acos(inner_product / modulus);
	}

	double linear_length(const CartesianCoord& c0, const CartesianCoord& c1) {
		double dist =
				sqrt((c0.x - c1.x) * (c0.x - c1.x) + (c0.y - c1.y) * (c0.y - c1.y) + (c0.z - c1.z) * (c0.z - c1.z));
		return dist;
	}

	double arc_length(const CartesianCoord& c0, const CartesianCoord& c1) {
		double innerProduct = (c0.x * c1.x + c0.y * c1.y + c0.z * c1.z);
		double modulus = sqrt(c0.x * c0.x + c0.y * c0.y + c0.z * c0.z) *
				sqrt(c1.x * c1.x + c1.y * c1.y + c1.z * c1.z);

		double arcRad = acos(innerProduct / modulus);
		return arcRad * EQ_RADIUS;
	}

	unsigned int predict_domain(const CartesianCoord& c) {
		Icosahedron ico;

		int apart = 9;

		if (point_above_plane(c, ico.p5, ico.p9) != -1 && point_above_plane(c, ico.p9, ico.p11) != -1
			&& point_above_plane(c, ico.p11, ico.p10) == 1 && point_above_plane(c, ico.p10, ico.p5) == 1) {
			apart = 8;
		}

		else if (point_above_plane(c, ico.p4, ico.p8) != -1 && point_above_plane(c, ico.p8, ico.p11) != -1
			&& point_above_plane(c, ico.p11, ico.p9) == 1 && point_above_plane(c, ico.p9, ico.p4) == 1) {
			apart = 7;
		}

		else if (point_above_plane(c, ico.p3, ico.p7) != -1 && point_above_plane(c, ico.p7, ico.p11) != -1
			&& point_above_plane(c, ico.p11, ico.p8) == 1 && point_above_plane(c, ico.p8, ico.p3) == 1) {
			apart = 6;
		}

		else if (point_above_plane(c, ico.p2, ico.p6) != -1 && point_above_plane(c, ico.p6, ico.p11) != -1
			&& point_above_plane(c, ico.p11, ico.p7) == 1 && point_above_plane(c, ico.p7, ico.p2) == 1) {
			apart = 5;
		}

		else if (point_above_plane(c, ico.p0, ico.p5) != -1 && point_above_plane(c, ico.p5, ico.p10) != -1
			&& point_above_plane(c, ico.p10, ico.p1) == 1 && point_above_plane(c, ico.p1, ico.p0) == 1) {
			apart = 4;
		}

		else if (point_above_plane(c, ico.p0, ico.p4) != -1 && point_above_plane(c, ico.p4, ico.p9) != -1
			&& point_above_plane(c, ico.p9, ico.p5) == 1 && point_above_plane(c, ico.p5, ico.p0) == 1) {
			apart = 3;
		}

		else if (point_above_plane(c, ico.p0, ico.p3) != -1 && point_above_plane(c, ico.p3, ico.p8) != -1
			&& point_above_plane(c, ico.p8, ico.p4) == 1 && point_above_plane(c, ico.p4, ico.p0) == 1) {
			apart = 2;
		}

		else if (point_above_plane(c, ico.p0, ico.p2) != -1 && point_above_plane(c, ico.p2, ico.p7) != -1
			&& point_above_plane(c, ico.p7, ico.p3) == 1 && point_above_plane(c, ico.p3, ico.p0) == 1) {
			apart = 1;
		}

		else if (point_above_plane(c, ico.p0, ico.p1) != -1 && point_above_plane(c, ico.p1, ico.p6) != -1
			&& point_above_plane(c, ico.p6, ico.p2) == 1 && point_above_plane(c, ico.p2, ico.p0) == 1) {
			apart = 0;
		}
		else if (abs(c.z - EQ_RADIUS) <= EQ_EPS) {
			apart = 4;
		}

		return apart;
	}

	void get_domain_corner(const int& domain, CartesianCoord& cc0, CartesianCoord& cc1,
	                       CartesianCoord& cc2, CartesianCoord& cc3) {
		Icosahedron ico;
		switch (domain) {
			case 0: {
				cc0 = ico.p0;
				cc1 = ico.p2;
				cc2 = ico.p6;
				cc3 = ico.p1;
			}
				break;
			case 1: {
				cc0 = ico.p0;
				cc1 = ico.p3;
				cc2 = ico.p7;
				cc3 = ico.p2;
			}
				break;
			case 2: {
				cc0 = ico.p0;
				cc1 = ico.p4;
				cc2 = ico.p8;
				cc3 = ico.p3;
			}
				break;
			case 3: {
				cc0 = ico.p0;
				cc1 = ico.p5;
				cc2 = ico.p9;
				cc3 = ico.p4;
			}
				break;
			case 4: {
				cc0 = ico.p0;
				cc1 = ico.p1;
				cc2 = ico.p10;
				cc3 = ico.p5;
			}
				break;
			case 5: {
				cc0 = ico.p2;
				cc1 = ico.p7;
				cc2 = ico.p11;
				cc3 = ico.p6;
			}
				break;
			case 6: {
				cc0 = ico.p3;
				cc1 = ico.p8;
				cc2 = ico.p11;
				cc3 = ico.p7;
			}
				break;
			case 7: {
				cc0 = ico.p4;
				cc1 = ico.p9;
				cc2 = ico.p11;
				cc3 = ico.p8;
			}
				break;
			case 8: {
				cc0 = ico.p5;
				cc1 = ico.p10;
				cc2 = ico.p11;
				cc3 = ico.p9;
			}
				break;
			case 9: {
				cc0 = ico.p1;
				cc1 = ico.p6;
				cc2 = ico.p11;
				cc3 = ico.p10;
			}
				break;
			default: 
				assert(0);
			;
		}
	}

	CartesianCoord get_normal_vector(const CartesianCoord& start, const CartesianCoord& end) {
		double a1 = start.x;
		double a2 = start.y;
		double a3 = start.z;

		double b1 = end.x;
		double b2 = end.y;
		double b3 = end.z;

		CartesianCoord res;
		res.x = a2 * b3 - a3 * b2;
		res.y = a3 * b1 - a1 * b3;
		res.z = a1 * b2 - a2 * b1;

		double len = sqrt(res.x * res.x + res.y * res.y + res.z * res.z);

		res.x = res.x / len;
		res.y = res.y / len;
		res.z = res.z / len;

		return res;
	}

	double vectors_angle(const CartesianCoord& vA, const CartesianCoord& vB) {
		double angle = acos((vA.x * vB.x + vA.y * vB.y + vA.z * vB.z) /
		(sqrt(vA.x * vA.x + vA.y * vA.y + vA.z * vA.z)
			* sqrt(vB.x * vB.x + vB.y * vB.y + vB.z * vB.z)));

		return angle;
	}

	int point_above_plane(const CartesianCoord& pt, const CartesianCoord& pA, const CartesianCoord& pB) {
		CartesianCoord normal = get_normal_vector(pA, pB);

		double angle = vectors_angle(normal, pt);
		double minus = angle - EQ_PI / 2.;

		if (minus < -EQ_EPS) {
			return 1;
		}
		if (abs(minus) <= EQ_EPS) {
			return 0;
		}
		return -1;
	}

	int point_in_diamond(const CartesianCoord& pt, const Diamond& d) {
		if (point_above_plane(pt, d.c1, d.c0) == 1
			&& point_above_plane(pt, d.c2, d.c1) == 1
			&& point_above_plane(pt, d.c3, d.c2) != -1
			&& point_above_plane(pt, d.c0, d.c3) != -1) {
			return 1;
		}

		return 0;
	}

	CartesianCoord mid_great_arc(const CartesianCoord& c1, const CartesianCoord& c2) {
		CartesianCoord c;
		c.x = (c1.x + c2.x) * 0.5;
		c.y = (c1.y + c2.y) * 0.5;
		c.z = (c1.z + c2.z) * 0.5;

		double length = sqrt(c.x * c.x + c.y * c.y + c.z * c.z);
		c.x = c.x / length * EQ_RADIUS;
		c.y = c.y / length * EQ_RADIUS;
		c.z = c.z / length * EQ_RADIUS;

		return c;
	}

	CartesianCoord mid_lon_lat(const CartesianCoord& c1, const CartesianCoord& c2) {
		CartesianCoord mid_axis = (CartesianCoord(c1) + c2) * 0.5;

		double mid_axis_length = sqrt(mid_axis.x * mid_axis.x + mid_axis.y * mid_axis.y);
		double square_of_radius_dir = mid_axis.z * mid_axis.z;
		double radius_horizental = sqrt(EQ_RADIUS * EQ_RADIUS - square_of_radius_dir);
		CartesianCoord mid_arc(mid_axis.x * radius_horizental / mid_axis_length, mid_axis.y * radius_horizental / mid_axis_length
		                       , mid_axis.z);

		return mid_arc;
	}

	CartesianCoord normalize(const CartesianCoord& c, const double& r) {
		double len = sqrt(c.x * c.x + c.y * c.y + c.z * c.z);
		CartesianCoord ret = c * (r / len);
		return ret;
	}

	CartesianCoord cross(const CartesianCoord& c1, const CartesianCoord& c2) {
		CartesianCoord normal_vec;
		normal_vec.x = c1.y * c2.z - c1.z * c2.y;
		normal_vec.y = c1.z * c2.x - c1.x * c2.z;
		normal_vec.z = c1.x * c2.y - c1.y * c2.x;

		return normal_vec;
	}

	CartesianCoord intersection(const CartesianCoord& c00, const CartesianCoord& c01,
	                            const CartesianCoord& c10, const CartesianCoord& c11) {
		CartesianCoord n0 = get_normal_vector(c00, c01);
		CartesianCoord n1 = get_normal_vector(c10, c11);
		CartesianCoord inter = get_normal_vector(n1, n0);

		return normalize(inter, EQ_RADIUS);
	}

	CartesianCoord rotate_vector_by_axis(const CartesianCoord& v, const CartesianCoord& axis, const double& angle) {
		double c = cos(angle);
		double s = sin(angle);
		double x = axis.x, y = axis.y, z = axis.z;
		double px = v.x, py = v.y, pz = v.z;

		/* rotation matrix
		|RM00 RM01 RM11|    |x|
		|RM10 RM11 RM12| *  |y|
		|RM20 RM21 RM22|    |z|
		*/
		double RM00 = x * x * (1 - c) + c;
		double RM01 = x * y * (1 - c) - z * s;
		double RM02 = x * z * (1 - c) + y * s;
		double RM10 = y * x * (1 - c) + z * s;
		double RM11 = y * y * (1 - c) + c;
		double RM12 = y * z * (1 - c) - x * s;
		double RM20 = x * z * (1 - c) - y * s;
		double RM21 = y * z * (1 - c) + x * s;
		double RM22 = z * z * (1 - c) + c;

		return CartesianCoord(px * RM00 + py * RM01 + pz * RM02,
		                      px * RM10 + py * RM11 + pz * RM12,
		                      px * RM20 + py * RM21 + pz * RM22);
	}

	void recursive_split(CartesianCoord** ccList, const Diamond& d,
	                     EQ_UINT index0I, EQ_UINT index0J, EQ_UINT index1I, EQ_UINT index1J,
	                     EQ_UINT index2I, EQ_UINT index2J, EQ_UINT index3I, EQ_UINT index3J,
	                     const EQ_UINT& level) {
		if (ccList) {
			ccList[index0I][index0J] = d.c0;
			ccList[index1I][index1J] = d.c1;
			ccList[index2I][index2J] = d.c2;
			ccList[index3I][index3J] = d.c3;
		}

		if (level <= 0) {
			return;
		}

		CartesianCoord m0, m1, m2, m3, center;
		diamond_mids(d, m0, m1, m2, m3);
		center = diamond_center(d);

		if (ccList) {
			ccList[(index0I + index3I) / 2][index0J] = m3;
			ccList[index0I][(index0J + index1J) / 2] = m0;
			ccList[(index0I + index3I) / 2][index1J] = m1;
			ccList[index3I][(index0J + index1J) / 2] = m2;
			ccList[(index0I + index3I) / 2][(index0J + index1J) / 2] = center;

			recursive_split(ccList, Diamond(d.c0, m0, center, m3),
			                index0I, index0J, index0I, (index0J + index1J) / 2,
			                (index0I + index3I) / 2, (index0J + index1J) / 2, (index0I + index3I) / 2, index0J,
			                level - 1);
			recursive_split(ccList, Diamond(m0, d.c1, m1, center),
			                index0I, (index0J + index1J) / 2, index1I, index1J,
			                (index0I + index3I) / 2, index1J, (index0I + index3I) / 2, (index0J + index1J) / 2,
			                level - 1);
			recursive_split(ccList, Diamond(center, m1, d.c2, m2),
			                (index0I + index3I) / 2, (index0J + index1J) / 2, (index0I + index3I) / 2, index1J,
			                index2I, index2J, index3I, (index0J + index1J) / 2,
			                level - 1);
			recursive_split(ccList, Diamond(m3, center, m2, d.c3),
			                (index0I + index3I) / 2, index0J, (index0I + index3I) / 2, (index0J + index1J) / 2,
			                index3I, (index0J + index1J) / 2, index3I, index3J,
			                level - 1);
		}
	}

	CartesianCoord diamond_center(const Diamond& d) {
		return mid_great_arc(d.c1, d.c3);
	}

	void diamond_mids(const Diamond& d,
	                  CartesianCoord& m0, CartesianCoord& m1, CartesianCoord& m2, CartesianCoord& m3) {
		m0 = mid_great_arc(d.c0, d.c1);
		m1 = mid_great_arc(d.c1, d.c2);
		m2 = mid_great_arc(d.c2, d.c3);
		m3 = mid_great_arc(d.c3, d.c0);
	}

	void split_arc_to_npart(const CartesianCoord& sp, const CartesianCoord& ep, CartesianCoord* splitList, const int& nPart) {
		double angle = vectors_angle(sp, ep);
		double angleUnit = angle / (nPart - 1);

		CartesianCoord normal = get_normal_vector(sp, ep);
		splitList[0] = sp;
		for (int i = 1; i < nPart - 1; i++) {
			CartesianCoord nc = rotate_vector_by_axis(sp, normal, angleUnit * i);
			splitList[i] = nc;
		}
		splitList[nPart - 1] = ep;
	}
}}
