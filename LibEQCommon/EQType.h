#ifndef EQTYPE_H
#define EQTYPE_H

#include "EQDef.h"

namespace eqtm {
#ifndef SPHERICCOORD_STRUCT
#define SPHERICCOORD_STRUCT
	struct SphericCoord {
		double longitude;
		double latitude;

		explicit SphericCoord(const double& slon = 0.0, const double& slat = 0.0)
			: longitude(slon), latitude(slat) {
		}

		SphericCoord operator+(const SphericCoord& rhs) const {
			auto absolute = [](const double& val) {
				return val < 0. ? -val : val;
			};

			if (absolute(latitude - 90.) < EQ_EPS || absolute(latitude + 90.) < EQ_EPS) {
				return SphericCoord{rhs.longitude + rhs.longitude, latitude + rhs.latitude};
			}

			if (absolute(rhs.latitude - 90.) < EQ_EPS || absolute(rhs.latitude + 90.) < EQ_EPS) {
				return SphericCoord{longitude + longitude, latitude + rhs.latitude};
			}
			return SphericCoord{longitude + rhs.longitude, latitude + rhs.latitude};
		}

		SphericCoord operator/(const double& rhs) const {
			return SphericCoord{longitude / rhs, latitude / rhs};
		}
	};
#endif

#ifndef EQCODE_STRUCT
#define EQCODE_STRUCT
	// D means grid's domains
	// note: 0-9 for cell and edge
	//       but for node 10 for north pole and 11 for south pole
	// T means grid element's type
	// note: 0000 represents node
	//       0001,0010,0011 represents edge of nw,center,sw
	//       0100,0101 represents upper and bottom cell
	// M means morton code
	// note: size is 16 bytes
	struct EQCode {
		EQ_UINT dt : 8;
		EQ_UINT len : 6;
		EQ_ULLONG morton : 64;

		inline bool operator <(const EQCode& rhs) const { // for map
			if (len < rhs.len) {
				return true;
			}

			if (len > rhs.len) {
				return false;
			}

			if ((dt >> 4) < (rhs.dt >> 4)) {
				return true;
			}

			if ((dt >> 4) > (rhs.dt >> 4)) {
				return false;
			}

			if (morton < rhs.morton) {
				return true;
			}

			if (morton > rhs.morton) {
				return false;
			}

			if ((dt & 0x0f) < (rhs.dt & 0x0f)) {
				return true;
			}

			if ((dt & 0x0f) > (rhs.dt & 0x0f)) {
				return false;
			}

			return false;
		}
	};
#endif

#ifndef ELEMENTTYPE_ENUM
#define ELEMENTTYPE_ENUM

	// grid's component element
	enum ElementType {
		EQ_NODE, // grid node
		EQ_EDGE, // grid edge
		EQ_CELL, // grid cell
	};
#endif

#ifndef DATATYPE_ENUM
#define DATATYPE_ENUM

	enum DataType {
		POINT,
		LINE,
		POLYGON,
		RASTER,
	};
#endif

	template <typename T, unsigned N>
	class Geom {
	public:
		T& v(int i) {
			return m_v[i];
		}

		const T& v(int i) const {
			return m_v[i];
		}

	private:
		T m_v[N];
	};

	typedef Geom<SphericCoord, 3> Trigon;

#ifndef POINT_STRUCT
#define POINT_STRUCT

	struct Point {
		EQCode code;
		EQ_UINT id;
	};
#endif

#ifndef LINE_STRUCT
#define LINE_STRUCT

	struct Line {
		Point* points;
		EQ_UINT size;
		EQ_UINT id;
	};
#endif

#ifndef POLYGON_STRUCT
#define POLYGON_STRUCT

	struct Polygon {
		Line exring;
		Line* inring;
		EQ_UINT innersize;
		EQ_UINT id;
	};
#endif

#ifndef EXTENT_STRUCT
#define EXTENT_STRUCT

	struct Extent {
		EQCode topleft;
		EQCode topright;
		EQCode botleft;
		EQCode botright;
	};
#endif
}

#endif
