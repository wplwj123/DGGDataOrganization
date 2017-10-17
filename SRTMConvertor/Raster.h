#pragma once
class GDALRasterBand;

class CRaster{
public:
	// cons and des
	CRaster();
	~CRaster();

	// delete copy
	CRaster(const CRaster&) = delete;
	CRaster& operator=(const CRaster&) = delete;

	// initialize raster dataset
	void initial(const float & xTopLeft, const float & yTopLeft,
		const float & cellSize, const int & nRow, const int & nCol,
		const int & nBand = 0, const float & noDataValue = 99999.f,
		float max = 0.f, float min = 0.f);

	// set raster's one band data
	void setBand(const int& band = 0, const float* bandData = nullptr);

	// get raster's one band data
	float* getBand(const int& band = 0);

	// get top left corner
	float getXTopLeft() const;
	float getYTopLeft() const;

	// get pixel size
	float getCellSize() const;

	// get rows and columns
	int getNRow() const;
	int getNCol() const;

	// get count of band
	int getNBand() const;

	// nodata value
	float getNoDataValue(const int& band = 0) const;
	void SetNoDataValue(const int& band = 0, const float& noDataValue = 99999.f);

	// set the pixel value at the position
	void setValue(const int& row, const int& col, const int& band = 0, 
		const float& v = 0.f);

	// get the pixel value using lon and lat
	float getValue(const float& x, const float& y, const int& band = 0)const;

	// get the pixel value
	float getValue(const int& row, const int& col, const int& band = 0)const;

	float getValue(const int& row, const int& col, const float& defaultValue, const int& band = 0)const;

	// get the value of the grid point in lat/lon format
	// attention!!!band nun is index from 0
	float getValueFromBilinearInterpolation(const float &x, const float &y,
		const int &band = 0)const;

	// get the value of the grid point in lat/lon format
	// attention!!!band nun is index from 0
	float getValueFromBicubicInterpolatation(const float& x, const float& y,
		const int& band = 0)const;

	// judge if target cell is valid
	bool IsValidIndex(const int& row, const int& col)const;

	void setMax(const int& band = 0, const float& value = 0.f);
	void setMin(const int& band = 0, const float& value = 0.f);

	float* data();

	float getMax(const int& band = 0)const;
	float getMin(const int& band = 0)const;

private:
	// top left corner's lat/lon
	float m_xTopLeft;
	float m_yTopLeft;

	// square pixel's size
	float m_cellSize;
	int m_nRow;
	int m_nCol;

	// several bands may appear
	int m_nBand;
	float* m_noDataValues;
	float* m_dataPtr;

	// maxim value and minmal value
	float *m_max;
	float *m_min;
};

