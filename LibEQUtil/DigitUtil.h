/*****************************************************************************
* DigitUtil.h
*
* DESCRIPTION
* this namespace contains some utilities of geomatics
* attention!!!just for me not provided for others!
*
* NOTES
*****************************************************************************/

#ifndef DIGITUTIL_H
#define DIGITUTIL_H

#include "../LibEQCommon/EQDef.h"

#ifdef LIBEQUTIL_EXPORT
#define LIBEQUTIL_API __declspec(dllexport)
#else
#define LIBEQUTIL_API __declspec(dllimport)
#endif

namespace eqtm {
	namespace util {
		/**
		* decimal to quaternary char array.
		* @param quat returned quaternary array
		* @param decimal converted decimal number
		* @param len array length without end '\0', equals grid's level
		* note: actually its length is one char more than this
		* note: call decimal_to_binary firstly
		*/
		LIBEQUTIL_API void decimal_to_quaternary(EQ_UCHAR* quat,
			const EQ_ULLONG& decimal, const EQ_UINT& len);

		/**
		* quaternary char array to decimal.
		* @param quat input quaternary
		* note: char array must end with '\0'
		*/
		LIBEQUTIL_API EQ_ULLONG quaternary_to_decimal(const EQ_UCHAR* quat);

		/**
		* decimal to binary char array.
		* @param bin returned binary array
		* @param decimal converted decimal number
		* @param len array length without end '\0'
		* note: actually its length is one char more than this
		*/
		LIBEQUTIL_API void decimal_to_binary(EQ_UCHAR* bin,
			const EQ_ULLONG& decimal, const EQ_UINT& len);

		/**
		* binary char array to decimal.
		* @param bin returned decimal
		* note: char array must end with '\0'
		*/
		LIBEQUTIL_API EQ_ULLONG binary_to_decimal(const EQ_UCHAR* bin);

		/**
		* binary char array to row and col index.
		* @param bin input char array
		* @param i returned row index
		* @param j returned col index
		* note: bin char array must end with '\0'
		*/
		LIBEQUTIL_API void binary_to_ij(const EQ_UCHAR* bin, EQ_ULLONG& i, EQ_ULLONG& j);

		/**
		* row and col index to binary char array.
		* @param bin returned binary char array
		* @param i input row index
		* @param j input col index
		* @param len length of bin array
		* note: actually its length is one char more than this
		*/
		LIBEQUTIL_API void ij_to_binary(EQ_UCHAR* bin, 
			const EQ_ULLONG& i, const EQ_ULLONG& j, const EQ_UINT& len);

		template<typename T>
		T eq_pow(const T& base, const EQ_UINT& exp) {
			if (exp == 0) {
				return 1;
			}
			return base* eq_pow(base, exp - 1);
		}
		/**
		* pow.
		* note: ullong uint
		*/
		template LIBEQUTIL_API EQ_ULLONG eq_pow<EQ_ULLONG>(const EQ_ULLONG& base, const EQ_UINT& exp);
		template LIBEQUTIL_API EQ_UINT eq_pow<EQ_UINT>(const EQ_UINT& base, const EQ_UINT& exp);
	}
}
#endif
