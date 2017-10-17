/*****************************************************************************
* GeoUtil.h
*
* DESCRIPTION
* this namespace contains some utilities of geomatics
* attention!!!just for me not provided for others!
*
* NOTES
*****************************************************************************/

#ifndef GEOUTIL_H
#define GEOUTIL_H

#include "../LibEQCommon/EQDef.h"

#ifdef LIBEQUTIL_EXPORT
#define LIBEQUTIL_API __declspec(dllexport)
#else
#define LIBEQUTIL_API __declspec(dllimport)
#endif

namespace eqtm {
	struct SphericCoord;
	struct CartesianCoord;
	struct Diamond;

	namespace util{
		/**
		* convert lat/lon to coordination in xyz form.
		* @param sc lat/lon
		* @return cartesian coordination
		*/
		LIBEQUTIL_API CartesianCoord spheric_to_cartesian(const SphericCoord& sc);

		/**
		* convert xyz to lat/lon form.
		* @param cc xyz coordination
		* @return lat/lon
		*/
		LIBEQUTIL_API SphericCoord cartesian_to_spheric(const CartesianCoord& cc);

		/**
		* angle between arms.
		* @param arm0 arm's endpoint
		* @param arm1 arm's endpoint
		* @param vertex arms' common point
		*/
		LIBEQUTIL_API double inner_angle(const CartesianCoord& arm0, const CartesianCoord& arm1, const CartesianCoord& vertex);

		/**
		* linear length between two points.
		* @param c0 start point
		* @param c1 end point
		*/
		LIBEQUTIL_API double linear_length(const CartesianCoord& c0, const CartesianCoord& c1);

		/**
		* great arc's length between two points.
		* @param c0 start point
		* @param c1 end point
		*/
		LIBEQUTIL_API double arc_length(const CartesianCoord & c0, const CartesianCoord & c1);

		/**
		* predict which domain this point locates in.
		* @param c the targeted point
		*/
		LIBEQUTIL_API EQ_UINT predict_domain(const CartesianCoord& c);

		/**
		* get initial diamond's vertices.
		* @param cc0 - cc3 points' coordination
		*/
		LIBEQUTIL_API void get_domain_corner(const int& domain, CartesianCoord& cc0, CartesianCoord& cc1,
			CartesianCoord& cc2, CartesianCoord& cc3);

		/**
		* get normal vector of the face composed of two points on sphere and center of sphere.
		* @param start start point on sphere
		* @param end end point on sphere
		*/
		LIBEQUTIL_API CartesianCoord get_normal_vector(const CartesianCoord& start, const CartesianCoord& end);

		/**
		* angle of 2 vectors.
		* @param vA 3D vector from {0,0,0}
		* @param vB 3D vector from {0,0,0}
		*/
		LIBEQUTIL_API double vectors_angle(const CartesianCoord& vA, const CartesianCoord& vB);

		/**
		* whether this point is above this plane.
		* @param pt the targeted point
		* @param pA face's point
		* @param pB face's point
		* @return 1 is above, 0 is in and -1 is under
		* 
		* right hand rule
		*/
		LIBEQUTIL_API int point_above_plane(const CartesianCoord& pt, const CartesianCoord& pA, const CartesianCoord& pB);

		/**
		* whether this point is in this diamond.
		* @param pt the targeted point
		* @param d the diamond
		* @return 1 is in and 0 is out or on the right boundary
		*/
		LIBEQUTIL_API int point_in_diamond(const CartesianCoord& pt, const Diamond& d);

		/**
		* middle point of great arc.
		*/
		LIBEQUTIL_API CartesianCoord mid_great_arc(const CartesianCoord& c1, const CartesianCoord& c2);

		/**
		* middle of lat/lon.
		*/
		LIBEQUTIL_API CartesianCoord mid_lon_lat(const CartesianCoord& c1, const CartesianCoord& c2);

		/**
		* normalize the point or vector to the targeted length.
		* @param c point
		* @param r length
		*/
		LIBEQUTIL_API CartesianCoord normalize(const CartesianCoord& c, const double& r = 1.);

		/**
		* two vectors' cross product, or face's normal vector
		*/
		LIBEQUTIL_API CartesianCoord cross(const CartesianCoord&c1, const CartesianCoord& c2);

		/**
		* intersection of two arcs
		*/
		LIBEQUTIL_API CartesianCoord intersection(const CartesianCoord& c00, const CartesianCoord& c01,
			const CartesianCoord& c10, const CartesianCoord& c11);

		/**
		* rotate vector by the targeted axis and the angle
		*/
		LIBEQUTIL_API CartesianCoord rotate_vector_by_axis(const CartesianCoord& v, const CartesianCoord& axis, const double& angle);

		/**
		* get grid nodes in this initial diamond.
		* @param ccList returned grid nodes in ij array
		* @param d intial parent diamond
		* @param index0I - index3J array index in the direction of i and j
		* @param level grid's level
		*/
		LIBEQUTIL_API void recursive_split(CartesianCoord** ccList, const Diamond& d,
			EQ_UINT index0I, EQ_UINT index0J, EQ_UINT index1I, EQ_UINT index1J,
			EQ_UINT index2I, EQ_UINT index2J, EQ_UINT index3I, EQ_UINT index3J,
			const EQ_UINT& level);

		/**
		* get diamond's center's coordination.
		* @param d targeted diamond
		*/
		LIBEQUTIL_API CartesianCoord diamond_center(const Diamond& d);

		/**
		* get diamond's edges' middle points.
		* @param d targeted diamond
		* @param m0 - m4 middle points' coordination in clockwise direction
		*/
		LIBEQUTIL_API void diamond_mids(const Diamond& d,
			CartesianCoord& m0, CartesianCoord& m1, CartesianCoord& m2, CartesianCoord& m3);

		/**
		* split great arc into n part
		*/
		LIBEQUTIL_API void split_arc_to_npart(const CartesianCoord& sp, const CartesianCoord& ep, CartesianCoord* splitList, const int& nPart);
	}
}
#endif