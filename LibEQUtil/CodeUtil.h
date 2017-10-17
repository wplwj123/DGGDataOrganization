/*****************************************************************************
* CodeUtil.h
*
* DESCRIPTION
* this namespace contains some utilities of geomatics
* attention!!!just for me not provided for others!
*
* NOTES
*****************************************************************************/
#ifndef CODEUTIL_H
#define CODEUTIL_H

#include "../LibEQCommon/EQDef.h"

#ifdef LIBEQUTIL_EXPORT
#define LIBEQUTIL_API __declspec(dllexport)
#else
#define LIBEQUTIL_API __declspec(dllimport)
#endif

namespace eqtm {
	// forward declare
	struct EQCode;
	struct Diamond;
	struct CartesianCoord;
	enum ElementType;

	namespace util {
		/**
		* get point's domain and type.
		* @param cc the point's cartesian coordination
		* @param d final sub diamond
		* @param ele element identifier
		* @note: call get_nearest_edge or get_located_cell
		*/
		LIBEQUTIL_API EQ_UCHAR get_dt(const CartesianCoord& cc, const Diamond& d, const ElementType& ele);

		/**
		* get code's initial domain.
		* @param code grid's cdoe
		*/
		LIBEQUTIL_API EQ_UINT get_domian(const EQCode& code);

		/**
		* get code's element type identifier.
		* @param code grid's cdoe
		*/
		LIBEQUTIL_API ElementType get_type(const EQCode& code);

		/**
		* deprecated get point's morton code.
		* @param morton the morton code
		* @param cc the point's cartesian coordination
		* @param d initial parent diamond
		* @param level grid's level
		* @param count count the current index of morton code
		* you could ignore it for default value set
		*/
		LIBEQUTIL_API void get_morton(EQ_UCHAR* morton,
			const CartesianCoord& cc, Diamond& d, const EQ_UINT& level, EQ_UINT count = 0);

		/**
		* get point's morton code.
		* @param morton the morton code
		* @param cc the point's cartesian coordination
		* @param d initial parent diamond
		* @param level grid's level
		* @param count count the current index of morton code
		* you could ignore it for default value set
		*/
		LIBEQUTIL_API void get_morton(EQ_ULLONG& morton,
			const CartesianCoord& cc, Diamond& d, const EQ_UINT& level, EQ_UINT count = 0);

		/**
		* deprecated get final sub diamond.
		* @param d the located diamond, firstly initial parent diamond should be passed
		* @param morton the morton code
		* @param level grid's level
		* @param count count the current index of morton code
		* you could ignore it for default value set
		*/
		LIBEQUTIL_API void get_diamond(Diamond& d,
			EQ_UCHAR* morton, const EQ_UINT& level, EQ_UINT count = 0);

		/**
		* get final sub diamond.
		* @param d the located diamond, firstly initial parent diamond should be passed
		* @param morton the morton code
		* @param level grid's level
		* @param count count the current index of morton code
		* you could ignore it for default value set
		*/
		LIBEQUTIL_API void get_diamond(Diamond& d,
			const EQ_ULLONG& morton, const EQ_UINT& level, EQ_UINT count = 0);

		/**
		* get sub diamond.
		* @param d the parent diamond
		* @param subid sub diamond's morton id
		* note:
		               1
		             0   3
		               2
		*/
		LIBEQUTIL_API Diamond get_sub_diamond(const Diamond& d, const EQ_UCHAR& subid);

		/**
		* get sub diamond.
		* @param d the parent diamond
		* @param cc the point in the diamond, ignore checking now
		* @param subid sub diamond's morton id
		*/
		LIBEQUTIL_API Diamond get_sub_diamond(const Diamond&d, const CartesianCoord& cc, EQ_UCHAR& subid);

		/**
		* get element's lat/lon in this targeted sub diamond.
		* @param d the located sub diamond
		* @param dt domain and type, in fact type is only needed
		* @param ele its element identifier
		*/
		LIBEQUTIL_API CartesianCoord get_diamond_element(const Diamond& d, const EQ_UCHAR& dt, ElementType& ele);

		/**
		* get point's nearest edge's dt.
		* @param cc the point's cartesian coordination
		* @param d the targeted sub diamond
		* @param domain the point's located diamond domain
		*/
		LIBEQUTIL_API EQ_UCHAR get_nearest_edge(const CartesianCoord& cc, const Diamond& d, const EQ_UINT& domain);

		/**
		* get point's located triangle cell's dt.
		* @param cc the point's cartesian coordination
		* @param d the targeted sub diamond
		* @param domain the point's located diamond domain
		*/
		LIBEQUTIL_API EQ_UCHAR get_located_cell(const CartesianCoord& cc, const Diamond& d, const EQ_UINT& domain);
	}
}
#endif