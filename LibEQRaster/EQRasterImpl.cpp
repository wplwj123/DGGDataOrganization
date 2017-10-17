// project
#include "EQRasterImpl.h"

// solution
#include "../LibEQUtil/DigitUtil.h"
#include "../LibEQUtil/CodeUtil.h"

// c++
#include <cassert>
#include <string>

#ifdef _DEBUG
#pragma comment(lib,"../build/LibEQUtild.lib")
#else
#pragma comment(lib,"../build/LibEQUtil.lib")
#endif

void eqtm::EQRaster::release() {
	if (m_attrs.size()) {
		for (size_t i = 0; i < m_nBand; ++i) {
			for (unsigned int j = 0; j < getMBSCount(); ++j) {
				if (m_mbs.count(j)>0) {
					m_attrs[i][j].clear();
					m_attrs[i][j].shrink_to_fit();
				}
			}
			m_attrs[i].clear();
		}
		m_attrs.clear();
		m_attrs.shrink_to_fit();
	}

	m_mbs.clear();
}

void eqtm::EQRaster::setHeader(const DataType& rid,
							   const ElementType& eid,
							   const unsigned& level,
							   const unsigned& nband, const float& nodata) {
	// header file
	m_dataID = rid;
	m_eleID = eid;
	m_level = level;
	m_nBand = nband;
	m_nodata = nodata;

	m_attrs.resize(m_nBand);
}

void eqtm::EQRaster::getHeader(DataType& rid,
							   ElementType& eid,
							   unsigned& level,
							   unsigned& nband, float& nodata) {
	rid = m_dataID;
	eid = m_eleID;
	level = m_level;
	nband = m_nBand;
	nodata = m_nodata;
}

void eqtm::EQRaster::setMBS(const EQ_UINT& band,
							const EQCode& offset,
							const unsigned long long& size) {
	// offset and size
#ifdef PYRAMID
	EQ_UINT domain = 0;
#else
	EQ_UINT domain = util::get_domian(offset);
#endif

	MinBoundSeg mbs = {offset, size};
	m_mbs.insert(std::pair<EQ_UINT, MinBoundSeg>(domain, mbs));

	m_attrs[band].insert(
		std::pair<EQ_UINT, vector<float>>(domain, vector<float>(size, 0.f)));
}

void eqtm::EQRaster::getMBS(const EQ_UINT& band,
							const unsigned& domain,
							EQCode& offset, unsigned long long& size) {
	if (m_mbs.count(domain) > 0) {
		MinBoundSeg mbs = m_mbs[domain];
		offset = mbs.offset;
		size = mbs.size;
	}
	else {
		size = 0;
	}
}

size_t eqtm::EQRaster::getMBSCount(){
	return m_mbs.size();
}

void eqtm::EQRaster::setAttribute(const unsigned& band,
								  const EQCode& code,
								  const float attr) {
	// offset and size
#ifdef PYRAMID
	EQ_UINT domain = 0;
#else
	EQ_UINT domain = util::get_domian(code);
#endif

	if (m_mbs.count(domain) <= 0) {
		assert(0);
	}

	// position
	size_t pos = 0;
	switch (m_eleID) {
		case EQ_CELL:
			pos = (code.morton - m_mbs[domain].offset.morton) * 2 +
					(code.dt - m_mbs[domain].offset.dt);
			break;
		case EQ_NODE:
			pos = (code.morton - m_mbs[domain].offset.morton) +
					(code.dt - m_mbs[domain].offset.dt);
			break;

		case EQ_EDGE:
			pos = (code.morton - m_mbs[domain].offset.morton) * 3 +
					(code.dt - m_mbs[domain].offset.dt);
			break;
		default:
			break;
	}
	// attributes
	m_attrs[band][domain][pos] = attr;
}

float eqtm::EQRaster::getAttribute(const unsigned& band,
								   const EQCode& code) {
#ifdef PYRAMID
	EQ_UINT domain = 0;
#else
	EQ_UINT domain = util::get_domian(code);
#endif

	if (m_mbs.count(domain) <= 0) {
		assert(0);
	}

	size_t pos = 0;
	switch (m_eleID) {
		case EQ_CELL:
			pos = (code.morton - m_mbs[domain].offset.morton) * 2 +
					(code.dt - m_mbs[domain].offset.dt);
			break;
		case EQ_NODE:
			pos = (code.morton - m_mbs[domain].offset.morton) +
					(code.dt - m_mbs[domain].offset.dt);
			break;

		case EQ_EDGE:
			pos = (code.morton - m_mbs[domain].offset.morton) * 3 +
					(code.dt - m_mbs[domain].offset.dt);
			break;
		default:
			break;
	}

	return m_attrs[band][domain][pos];
}

bool eqtm::EQRaster::isEmpty(){
	for each (auto band in this->m_attrs){
		for each (auto attrs in band){
			for each (auto attr in attrs.second){
				if (attr != this->m_nodata){
					return false;
				}
			}
		}
	}
	return true;
}
