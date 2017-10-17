#ifndef EQCODEC_H
#define EQCODEC_H

#include "EQDef.h"

#ifdef LIBEQCOMMON_EXPORT
#define LIBEQCOMMON_API __declspec(dllexport)
#else
#define LIBEQCOMMON_API __declspec(dllimport)
#endif

namespace eqtm {
	// forward declare
	struct EQCode;
	struct SphericCoord;
	enum ElementType;

	/**
	* convert lat/lon into e-qtm code.
	* @param sc the point's lat/lon
	* @param level grid's level
	* @param ele element identifier
	* @return EQTM code
	*/
	LIBEQCOMMON_API EQCode encode(const SphericCoord& sc,
	                              const EQ_UINT& level,
	                              const ElementType& ele);

	/**
	* convert EQTM code into lat/lon.
	* @param code the grid's code
	* @param level code's level
	* @param domain code's domain
	* @param ele element identifier
	* @return cell center's lat/lon
	*/
	LIBEQCOMMON_API SphericCoord decode(const EQCode& code,
	                                    EQ_UINT& domain,
	                                    EQ_UINT& level,
	                                    ElementType& ele);

	/**
	* convert EQTM cell code into trigon.
	* @param code the grid's code
	* @param level code's level
	* @param domain code's domain
	* @return cell's vertices in counter-clockwise order
	*/
	LIBEQCOMMON_API Trigon decode(const EQCode& code,
	                              EQ_UINT& domain,
	                              EQ_UINT& level);

	LIBEQCOMMON_API void getSubCells(const eqtm::EQCode& cell,
	                                 eqtm::EQCode& cell0,
	                                 eqtm::EQCode& cell1,
	                                 eqtm::EQCode& cell2,
	                                 eqtm::EQCode& cell3);

	LIBEQCOMMON_API EQCode getParentCell(const eqtm::EQCode& cell);

	// TODO:
	// find neighbor cells
}
#endif
