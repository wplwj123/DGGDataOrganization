#ifndef IEQRASTER_H
#define IEQRASTER_H

#include "../LibEQCommon/EQDef.h"

#ifdef LIBEQRASTER_EXPORT
#define LIBEQRASTER_API __declspec(dllexport)

#else
#define LIBEQRASTER_API __declspec(dllimport)
#endif

namespace eqtm {
	struct EQCode;
	struct Element;

	enum ElementType;
	enum DataType;

	/**
	* raster-grid dataset.
	* @note: not responsible for grid's generation
	*/
	class IEQRaster {
	public:
		virtual void release() = 0;

		virtual void setHeader(const DataType& rid,
		                       const ElementType& eid,
		                       const EQ_UINT& level,
		                       const EQ_UINT& nband, const float& nodata) = 0;

		virtual void getHeader(DataType& rid,
		                       ElementType& eid,
		                       EQ_UINT& level,
		                       EQ_UINT& nband, float& nodata) = 0;

		// set one domain's minimal bounding segment
		virtual void setMBS(const EQ_UINT& band,
		                    const EQCode& offset,
		                    const EQ_ULLONG& size) = 0;

		virtual void getMBS(const EQ_UINT& band,
		                    const EQ_UINT& domain,
		                    EQCode& offset,
		                    EQ_ULLONG& size) = 0;

		virtual size_t getMBSCount() = 0;

		// deep copy, remember to release attr!
		virtual void setAttribute(const EQ_UINT& band,
		                          const EQCode& code,
		                          const float attribute) = 0;

		// shallow copy, new is not needed
		virtual float getAttribute(const EQ_UINT& band,
		                           const EQCode& code) = 0;

		//return true if all attribute is nodata
		virtual bool isEmpty() = 0;
	};

	extern "C" LIBEQRASTER_API IEQRaster* _stdcall CreateRasterObject();

	extern "C" LIBEQRASTER_API void _stdcall DestroyRasterObject(IEQRaster* eraster);
}
#endif
