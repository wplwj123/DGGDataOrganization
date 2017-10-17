#ifndef TYPEUTIL_H
#define TYPEUTIL_H

namespace eqtm {
#ifndef CARTESIANCOORD_STRUCT
#define CARTESIANCOORD_STRUCT
	struct CartesianCoord{
		double x;
		double y;
		double z;

		explicit CartesianCoord(const double& cx = 0.0, const double& cy = 0.0, const double& cz = 0.0) :
			x(cx), y(cy), z(cz) {
		}

	    CartesianCoord operator*(const double& rhs)const{
			return CartesianCoord(x*rhs, y*rhs, z*rhs);
		}

		CartesianCoord operator+(const CartesianCoord& rhs)const{
			return CartesianCoord(x + rhs.x, y + rhs.y, z + rhs.z);
		}
	};
#endif

#ifndef DIAMOND_STRUCT
#define DIAMOND_STRUCT
	struct Diamond {
		CartesianCoord c0; // north
		CartesianCoord c1; // east
		CartesianCoord c2; // south
		CartesianCoord c3; // west

		Diamond(const CartesianCoord& v0, const CartesianCoord& v1,
			const CartesianCoord& v2,
			const CartesianCoord& v3) :c0(v0), c1(v1), c2(v2), c3(v3) {
		}
	};
#endif

#ifndef ICOSAHEDRON_STRUCT
#define ICOSAHEDRON_STRUCT
	struct Icosahedron{
		CartesianCoord p0;
		CartesianCoord p1;
		CartesianCoord p2;
		CartesianCoord p3;
		CartesianCoord p4;
		CartesianCoord p5;
		CartesianCoord p6;
		CartesianCoord p7;
		CartesianCoord p8;
		CartesianCoord p9;
		CartesianCoord p10;
		CartesianCoord p11;
		
		Icosahedron() :p0(0.0000000000, 0.0000000000, 6400000.0000000000),
			p1(5724334.0223994618, 0.0000000000, 2862167.0111997304),
			p2(1768916.4944001348, 5444165.1734530553, 2862167.0111997304),
			p3(-4631083.5055998648, 3364679.1175624561, 2862167.0111997304),
			p4(-4631083.5055998657, -3364679.1175624547, 2862167.0111997304),
			p5(1768916.4944001336, -5444165.1734530563, 2862167.0111997304),
			p6(4631083.5055998657, 3364679.1175624551, -2862167.0111997304),
			p7(-1768916.4944001341, 5444165.1734530563, -2862167.0111997304),
			p8(-5724334.0223994618, 0.0000000007, -2862167.0111997304),
			p9(-1768916.4944001355, -5444165.1734530553, -2862167.0111997304),
			p10(4631083.5055998648, -3364679.1175624565, -2862167.0111997304),
			p11(0.0000000000, 0.0000000000, -6400000.0000000000){}
	};
#endif
}
#endif