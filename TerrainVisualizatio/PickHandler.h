#pragma once

#include <osgViewer/ViewerEventHandlers>

class PickText;

namespace osgEarth{
	class GeoPoint;
}

class PickHandler : public osgGA::GUIEventHandler{

public:
	PickHandler(PickText* pText) :pText(pText){}
	~PickHandler(){}

	bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

private:
	osg::Vec3d XYtoCartesian(float x, float y, osg::ref_ptr<osgViewer::Viewer> viewer);
	osgEarth::GeoPoint CartesiantoSpheric(osg::Vec3d cc, osg::ref_ptr<osgViewer::Viewer> viewer);
	osgEarth::GeoPoint XYtoSpheric(float x, float y, osg::ref_ptr<osgViewer::Viewer> viewer);
	double getDistance(const osg::Vec3d& eye, const osg::Vec3d& pickPos);
	
private:
	PickText* pText;
};