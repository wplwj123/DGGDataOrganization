// project heades
#include "Raster.h"
#include "RasterReader.h"

// 3rd libs
#pragma warning(disable:4251)
#include <gdal_priv.h>

// c++
#include <cassert>

#ifdef _DEBUG
#pragma comment(lib,"gdal_i.lib")
#else
#pragma comment(lib,"gdal_i.lib")
#endif

CRaster* RasterReader::readRaster(const std::string& fileName, const int& band){
	int n_col = 0, n_row = 0;
	float xll_corner = 0.f, yll_corner = 0.f, cell_size = 0.f,
		nodata_value = 0.f, min_value = 0.f, max_value = 0.f;

	// register file format
	GDALAllRegister();
	const char * psz_file = fileName.c_str();
	GDALDataset *po_dataset = static_cast<GDALDataset*>(GDALOpen(psz_file, GA_ReadOnly));

	//check
	assert(po_dataset != nullptr);

	// image width and height
	n_col = po_dataset->GetRasterXSize();
	n_row = po_dataset->GetRasterYSize();

	// image corner coordinates and cell size
	double adf_geo_transform[6];
	if (po_dataset->GetGeoTransform(adf_geo_transform) == CE_None){
		// top left x and y
		xll_corner = static_cast<float>(adf_geo_transform[0]);
		yll_corner = static_cast<float>(adf_geo_transform[3]);
		cell_size = static_cast<float>(adf_geo_transform[1]);
	}

	// read the band
	// raster band starts with 1 raher 0
	int n_band = po_dataset->GetRasterCount();
	GDALRasterBand *po_band = po_dataset->GetRasterBand(band + 1);

	// check band
	assert(po_band != nullptr);

	// get max and min value
	int b_got_min = 0, b_got_max = 0;
	double adfMinMax[2];
	adfMinMax[0] = po_band->GetMinimum(&b_got_min);
	adfMinMax[1] = po_band->GetMaximum(&b_got_max);

	if (!(b_got_max&&b_got_min)){
		GDALComputeRasterMinMax(static_cast<GDALRasterBandH>(po_band), TRUE, adfMinMax);
	}

	min_value = static_cast<float>(adfMinMax[0]);
	max_value = static_cast<float>(adfMinMax[1]);
	nodata_value = static_cast<float>(po_band->GetNoDataValue());

	CRaster* raster_data = new CRaster;
	raster_data->initial(xll_corner, yll_corner,
		cell_size, n_row, n_col,
		1,
		nodata_value,
		max_value, min_value);

	float *band_data = nullptr;
	band_data = new float[po_band->GetYSize() * po_band->GetXSize()];
	po_band->RasterIO(GF_Read, 0, 0, po_band->GetXSize(), po_band->GetYSize(),
		band_data, po_band->GetXSize(), po_band->GetYSize(), GDT_Float32, 0, 0);
	raster_data->setBand(0, band_data);

	delete[]band_data;
	band_data = nullptr;

	delete po_dataset;

	return raster_data;
}