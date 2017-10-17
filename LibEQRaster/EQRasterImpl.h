#ifndef EQRASTERIMPL_H
#define EQRASTERIMPL_H

// prj
#include "IEQRaster.h"
// sln
#include "../LibEQCommon/EQType.h"
// cpp
#include <vector>
#include <map>

using std::vector;
using std::map;

namespace eqtm {
	struct MinBoundSeg {
		EQCode offset;
		EQ_ULLONG size;
	};

	class EQRaster :public IEQRaster {
	public:
		void release() override;

		void setHeader(const DataType& rid,
		               const ElementType& eid,
		               const EQ_UINT& level,
		               const EQ_UINT& nband, const float& nodata) override;

		void getHeader(DataType& rid,
		               ElementType& eid,
		               EQ_UINT& level,
		               EQ_UINT& nband, float& nodata) override;

		// set one domain's minimal bounding segment
		void setMBS(const EQ_UINT& band,
		            const EQCode& offset,
		            const EQ_ULLONG& size) override;

		void getMBS(const EQ_UINT& band,
		            const EQ_UINT& domain,
		            EQCode& offset,
		            EQ_ULLONG& size) override;

		size_t getMBSCount() override;

		void setAttribute(const EQ_UINT& band,
		                  const EQCode& code,
		                  const float attr) override;

		float getAttribute(const EQ_UINT& band,
			const EQCode& code) override;

		bool isEmpty() override;

	private:
		DataType m_dataID;
		ElementType m_eleID;
		EQ_UINT m_level;
		EQ_UINT m_nBand;

		// most raster data's bands share the same nodata value
		// so we only sore the one for a while
		float m_nodata;

		// 10 domains or less, same as nodata
		map<EQ_UINT, MinBoundSeg> m_mbs;

		// 3 dimensions include
		// band: dem has 1 band and image always has 3 bands
		// domain: 10 domains or less
		vector<map<EQ_UINT, vector<float>>> m_attrs;
	};
}
#endif
