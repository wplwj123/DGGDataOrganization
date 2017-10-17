#pragma once

#include <osg/ref_ptr>
#include <osgViewer/Viewer>
#include <string>  

namespace osgText {
	class Text;
}

namespace osgEarth{
	class GeoPoint;
}

class PickText
{
public:
	PickText();
	~PickText();

	osg::ref_ptr<osg::Camera> CreateLegend(int width, int height);
	void UpdataLegend(const osgEarth::GeoPoint& pickPoint, const double distance);

private:
	osg::ref_ptr<osgText::Text> CreateText(const std::wstring& t, int x, int y);

private:
	osg::ref_ptr<osgText::Text> lonText;
	osg::ref_ptr<osgText::Text> latText;
	osg::ref_ptr<osgText::Text> disText;
};

