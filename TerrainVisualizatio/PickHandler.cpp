#include "PickHandler.h"
#include "PickText.h"

#include <iostream>

#include <osgEarth/GeoData>

bool PickHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa){

	//convert aa to viewer 
	osg::ref_ptr<osgViewer::Viewer> viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
	if (!viewer)  {
		return false;
	}

	osg::Vec3d eye, center, up;
	viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);

	switch (ea.getEventType()){
		case osgGA::GUIEventAdapter::PUSH:
			if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON){
				osg::Vec3d pickPos = XYtoCartesian(ea.getX(), ea.getY(), viewer);
				pText->UpdataLegend(CartesiantoSpheric(pickPos, viewer), getDistance(eye, pickPos));
				
				//print camera parameter
				double left, right, bottom, top, zNear, zFar;
				if (viewer->getCamera()->getProjectionMatrixAsFrustum(left, right, bottom, top, zNear, zFar)){
					std::cout << "\nLeft: " << left << ", Right: " << right << std::endl;
					std::cout << "Bottom: " << bottom << ", Top: " << top << std::endl;
					std::cout << "zNear: " << zNear << ", zFar: " << zFar << std::endl;
					std::cout << "Distance: " << getDistance(eye, pickPos) << std::endl;
				}
			}
			break;
		case osgGA::GUIEventAdapter::RELEASE:
			if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON){
				osg::Vec3d pickPos = XYtoCartesian(ea.getX(), ea.getY(), viewer);
				pText->UpdataLegend(CartesiantoSpheric(pickPos, viewer), getDistance(eye, pickPos));
			}
			break;
		case osgGA::GUIEventAdapter::SCROLL:
			if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP){
				osg::Viewport* curViewport = viewer->getCamera()->getViewport();
				float centerX = curViewport->x() + curViewport->width() / 2;
				float centerY = curViewport->y() + curViewport->height() / 2;

				osg::Vec3d pickPos = XYtoCartesian(centerX, centerY, viewer);
				pText->UpdataLegend(CartesiantoSpheric(pickPos, viewer), getDistance(eye, pickPos));
			}
			else if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_DOWN){
				osg::Viewport* curViewport = viewer->getCamera()->getViewport();
				float centerX = curViewport->x() + curViewport->width() / 2;
				float centerY = curViewport->y() + curViewport->height() / 2;

				osg::Vec3d pickPos = XYtoCartesian(centerX, centerY, viewer);
				pText->UpdataLegend(CartesiantoSpheric(pickPos, viewer), getDistance(eye, pickPos));
			}
			break;
		default:
			break;
	}

	return false;
}

osg::Vec3d PickHandler::XYtoCartesian(float x, float y, osg::ref_ptr<osgViewer::Viewer> viewer){

	osgUtil::LineSegmentIntersector::Intersections hits;

	if (viewer->computeIntersections(x, y, hits)){
		return hits.begin()->getWorldIntersectPoint();
	}
	return osg::Vec3d(0, 0, 0);
}

osgEarth::GeoPoint PickHandler::CartesiantoSpheric(osg::Vec3d cc, osg::ref_ptr<osgViewer::Viewer> viewer){
	osgEarth::GeoPoint sc;

	if (cc != osg::Vec3d(0, 0, 0))
		sc.fromWorld(osgEarth::SpatialReference::get("wgs84", "egm96")->getGeographicSRS(), cc);
	return sc;
}

osgEarth::GeoPoint PickHandler::XYtoSpheric(float x, float y, osg::ref_ptr<osgViewer::Viewer> viewer){

	osg::Vec3d cc = XYtoCartesian(x, y, viewer);
	return CartesiantoSpheric(cc, viewer);
}

double PickHandler::getDistance(const osg::Vec3d& eye, const osg::Vec3d& pickPos){

	if (pickPos == osg::Vec3d(0, 0, 0))
		return -1;

	return std::sqrt((eye._v[0] - pickPos._v[0]) * (eye._v[0] - pickPos._v[0]) +
		(eye._v[1] - pickPos._v[1]) * (eye._v[1] - pickPos._v[1]) +
		(eye._v[2] - pickPos._v[2]) * (eye._v[2] - pickPos._v[2]));
}

