// project headers
#include "Raster.h"

// solution headers
#include "../LibEQUtil/GeoUtil.h"

// 3rd libs
#include <Eigen/Dense>

#ifdef _DEBUG
#pragma comment(lib,"../build/LibEQUtild.lib")
#else
#pragma comment(lib,"../build/LibEQUtil.lib")
#endif

using namespace Eigen;
using namespace eqtm;
using namespace eqtm::util;

inline double sinXDivx(double x);

CRaster::CRaster(){
	m_dataPtr = nullptr;
	m_noDataValues = nullptr;
	m_max = nullptr;
	m_min = nullptr;
}

CRaster::~CRaster(){
	if (m_dataPtr){
		delete[] m_dataPtr;
		m_dataPtr = nullptr;
	}

	if (m_noDataValues){
		delete[] m_noDataValues;
		m_noDataValues = nullptr;
	}

	if (m_max){
		delete[] m_max;
		m_max = nullptr;
	}

	if (m_min){
		delete[] m_min;
		m_min = nullptr;
	}
}

void CRaster::initial(const float & xTopLeft, const float & yTopLeft,
	const float & cellSize, const int & nRow, const int & nCol,
	const int & nBand, const float & noDataValue,
	float max, float min){
	m_xTopLeft = xTopLeft;
	m_yTopLeft = yTopLeft;
	m_cellSize = cellSize;
	m_nRow = nRow;
	m_nCol = nCol;
	m_nBand = nBand;
	m_dataPtr = new float[nRow * nCol * nBand];
	m_noDataValues = new float[nBand];
	m_max = new float[nBand];
	m_min = new float[nBand];

	for (int b = 0; b < nBand; ++b){
		m_noDataValues[b] = noDataValue;
		m_max[b] = max;
		m_min[b] = min;
	}
}

void CRaster::setBand(const int& band, const float* bandData) {
	memcpy(m_dataPtr + band*m_nCol*m_nRow, bandData, sizeof(float)*m_nCol*m_nRow);
}

float* CRaster::getBand(const int& band) {
	return m_dataPtr + band * m_nCol * m_nRow;
}

float CRaster::getXTopLeft() const {
	return m_xTopLeft;
}

float CRaster::getYTopLeft() const {
	return m_yTopLeft;
}

float CRaster::getCellSize() const {
	return m_cellSize;
}

int CRaster::getNRow() const {
	return m_nRow;
}

int CRaster::getNCol() const {
	return m_nCol;
}

int CRaster::getNBand() const {
	return m_nBand;
}

float CRaster::getNoDataValue(const int& band) const {
	return m_noDataValues[band];
}

void CRaster::SetNoDataValue(const int& band, const float& noDataValue) {
	m_noDataValues[band] = noDataValue;
}

void CRaster::setMax(const int& band, const float& value) {
	m_max[band] = value;
}

void CRaster::setMin(const int& band, const float& value) {
	m_min[band] = value;
}

float* CRaster::data() {
	return m_dataPtr;
}

float CRaster::getMax(const int& band)const{
	return m_max[band];
}

float CRaster::getMin(const int& band)const{
	return m_min[band];
}
#if 0
int CRaster::calculateLevelByLonLat(const float& cellSize){
	Icosahedron ico;

	double unit = (2 * EQ_PI * EQ_RADIUS / 360.0) * cellSize;
	double arcLen = arc_length(ico.p0, ico.p2);
	return static_cast<int>((arcLen / unit) / log(2.0));
}
#endif

void CRaster::setValue(const int& row, const int& col, const int& band, const float& v){
	if (IsValidIndex(row, col) & (band < m_nBand)){
		m_dataPtr[(col + getNCol() * row) * m_nBand + band] = v;
	}
}

float CRaster::getValue(const float& x, const float& y, const int& band)const{
	int col = static_cast<int>((x - m_xTopLeft) / m_cellSize);
	int row = static_cast<int>((m_yTopLeft - y) / m_cellSize);

#if 0
	if (m_xTopLeft < 0){
		col = (x - (m_xTopLeft + 360)) / m_cellSize;
		row = (m_yTopLeft - y) / m_cellSize;
	}
	else {
		col = (x - m_xTopLeft) / m_cellSize;
		row = (m_yTopLeft - y) / m_cellSize;
	}
#endif

	return getValue(row, col, band);
}

float CRaster::getValue(const int& row, const int& col, const int& band)const{
	if (IsValidIndex(row, col) && band < m_nBand)
		return m_dataPtr[(col + m_nCol * row) * m_nBand + band];

#if 0
	int tmpRow = row;
	int tmpCol = col;

	if (row >= getNRow()) tmpRow = getNRow() - 1;
	if (row < 0) tmpRow = 0;
	if (col >= getNCol()) tmpCol = getNCol() - 1;
	if (col < 0) tmpCol = 0;

	return m_dataPtr[(tmpCol + getNCol() * tmpRow) *m_nBand + band];
#endif
	else{
		return m_noDataValues[band];
	}
}

float CRaster::getValue(const int& row, const int& col,
	const float& defaultValue, const int& band) const {
	float ret;
	if (!IsValidIndex(row, col))	{
		ret = defaultValue;
	}
	else{
		ret = getValue(row, col);
	}

	return ret;
}

float CRaster::getValueFromBilinearInterpolation(const float &x, const float &y,
	const int &band)const{
	/************************************/
	/*(xTopLeft,yTopLeft)               */
	/*      q11 -------- q21            */
	/*       |     (u,v)  |             */
	/*       |            |             */
	/*      q12 -------- q22            */
	/*                                  */
	/************************************/
	float  q11, q21, q22, q12;
	if (x < m_xTopLeft || y > m_yTopLeft){
		return getNoDataValue(band);
	}

	int col = static_cast<int>((x - m_xTopLeft) / m_cellSize);
	int row = static_cast<int>((m_yTopLeft - y) / m_cellSize);

	float u = (x - m_xTopLeft - col * m_cellSize) / m_cellSize;
	float v = (m_yTopLeft - y - row * m_cellSize) / m_cellSize;

	if (!IsValidIndex(row, col) || band >= m_nBand){
		//return getNoDataValue(band);
		int tmpRow = row;
		int tmpCol = col;

		if (row >= getNRow())
			tmpRow = getNRow() - 1;

		if (row < 0) tmpRow = 0;

		if (col >= getNCol())
			tmpCol = getNCol() - 1;

		if (col < 0) tmpCol = 0;

		return getValue(tmpRow, tmpCol, 0);
	}

	else{
		q11 = getValue(row, col, band);
	}

	if (!IsValidIndex(row, col + 1) || band >= m_nBand)	{
		q12 = q11;
	}
	else{
		q12 = getValue(row, col + 1, band);
	}

	if (!IsValidIndex(row + 1, col) || band >= m_nBand){
		q21 = q11;
	}
	else{
		q21 = getValue(row + 1, col, band);
	}

	if (!IsValidIndex(row + 1, col + 1) || band >= m_nBand){
		q22 = q11;
	}
	else{
		q22 = getValue(row + 1, col + 1, band);
	}

	return static_cast<float>((1 - u) * (1 - v) * q11 + (1 - u) * v * q12
		+ u * (1 - v) * q21 + u * v * q22);
}

float CRaster::getValueFromBicubicInterpolatation(const float& x, const float& y,
	const int& band)const{
	/************************************************/
	/*(xTopLeft,yTopLeft)   						*/
	/*      p(-1,-1)   p(0,-1)    p(1,-1)   p(2,-1)	*/
	/*      p(-1,0)    *p(0,0)*   p(1,0)    p(2,0)	*/
	/*      p(-1,1)    p(0,1)     p(1,1)    p(2,1)	*/
	/*      p(-1,2)    p(0,2)     p(1,2)    p(2,2)  */
	/************************************************/
	if (x < m_xTopLeft || y > m_yTopLeft){
		return getNoDataValue(band);
	}

	int col = static_cast<int>((x - m_xTopLeft) / m_cellSize); // i
	int row = static_cast<int>((m_yTopLeft - y) / m_cellSize); // j

	float u = (x - m_xTopLeft - col * m_cellSize) / m_cellSize;
	float v = (m_yTopLeft - y - row * m_cellSize) / m_cellSize;

	Matrix<double, 1, 4> a;
	a << sinXDivx(u + 1), sinXDivx(u), sinXDivx(u - 1), sinXDivx(u - 2);

	float v00, v01, v02, v03;
	float v10, v11, v12, v13;
	float v20, v21, v22, v23;
	float v30, v31, v32, v33;

	// v11
	if (!IsValidIndex(row, col)){
		int tmpRow = row;
		int tmpCol = col;

		if (row >= getNRow())
			tmpRow = getNRow() - 1;

		if (row < 0) tmpRow = 0;

		if (col >= getNCol())
			tmpCol = getNCol() - 1;

		if (col < 0) tmpCol = 0;
		return getValue(tmpRow, tmpCol, 0);
	}
	else {
		v11 = getValue(row, col);
	}

	v00 = getValue(row - 1, col - 1, v11); // v00
	v01 = getValue(row, col - 1, v11); // v01
	v02 = getValue(row + 1, col - 1, v11); // v02
	v03 = getValue(row + 2, col - 1, v11); // v03
	v10 = getValue(row - 1, col, v11); // v10
	v12 = getValue(row + 1, col, v11); // v12
	v13 = getValue(row + 2, col, v11); // v13
	v20 = getValue(row - 1, col + 1, v11); // v20
	v21 = getValue(row, col + 1, v11); // v21
	v22 = getValue(row + 1, col + 1, v11); // v22
	v23 = getValue(row + 2, col + 1, v11); // v23
	v30 = getValue(row - 1, col + 2, v11); // v30
	v31 = getValue(row, col + 2, v11); // v31
	v32 = getValue(row + 1, col + 2, v11); // v32
	v33 = getValue(row + 2, col + 2, v11); // v33

	//float v00 = getValue(row - 1, col - 1), v01 = getValue(row, col - 1),
	//	v02 = getValue(row + 1, col - 1), v03 = getValue(row + 2, col - 1);
	//float v10 = getValue(row - 1, col), v11 = getValue(row, col),
	//	v12 = getValue(row + 1, col), v13 = getValue(row + 2, col);
	//float v20 = getValue(row - 1, col + 1), v21 = getValue(row, col + 1),
	//	v22 = getValue(row + 1, col + 1), v23 = getValue(row + 2, col + 2);
	//float v30 = getValue(row - 1, col + 2), v31 = getValue(row, col + 2),
	//	v32 = getValue(row + 1, col + 2), v33 = getValue(row + 2, col + 2);

	Matrix<double, 4, 4> b;
	b << v00, v01, v02, v03,
		v10, v11, v12, v13,
		v20, v21, v22, v23,
		v30, v31, v32, v33;

	Matrix<double, 4, 1> c;
	c << sinXDivx(v + 1),
		sinXDivx(v),
		sinXDivx(v - 1),
		sinXDivx(v - 2);

	return static_cast<float>(a*b*c);
}

bool CRaster::IsValidIndex(const int & row, const int & col)const{
	if (row < 0 || row >= getNRow() || col < 0 || col >= getNCol()){
		return false;
	}
	else
		return true;
}

double sinXDivx(double x){
#if 0
	if (x == 0){
		return 1;
	}
	return sin(x*PI) / x / PI;
#else
	const float a = -1; // adjust number to circumstance,such as -2, -1, -0.75, -0.5
	if (x < 0){
		x = -x;
	}

	double squareX = x*x;
	double cubicX = squareX*x;

	if (x <= 1){
		return (a + 2)*cubicX - (a + 3)*squareX + 1;
	}
	else if (x <= 2){
		return a*cubicX - 5 * a *squareX + (8 * a)*x - 4 * a;
	}
	else{
		return 0;
	}
#endif
}