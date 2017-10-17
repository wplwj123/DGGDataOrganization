#include "EQNodeRenderer.h"
#include "../LibEQCommon/EQType.h"
#include "../LibEQUtil/TypeUtil.h"
#include "../LibEQUtil/GeoUtil.h"

#include <osg/Point>
#include <osgGA/StateSetManipulator>
#include <osgUtil/Optimizer>
#include <osgViewer/config/SingleScreen>
#include <osgViewer/ViewerEventHandlers>
#include <osg/ShapeDrawable>

#include <osgEarth/GeoData>
#include <osgEarth/GeoCommon>

namespace eq = eqtm;
namespace equ = eqtm::util;

namespace {
	float gauss(float x, float y0, float a, float x0, float b) {
		float temp = y0 + a * exp(-(x0 - x) * (x0 - x) / (2 * b * b));
		return temp;
	}

	float circle(float x, float y0, float a, float b, float x0) {
		float temp = y0 + b * sqrt(1 - (x - x0) * (x - x0) / (a * a));
		return temp;
	}

	void gray_to_rgb(float gray, float& r, float& g, float& b) {
		r = gauss(gray, 0.00973, 0.95734, 0.68447, 0.40538);
		if (gray >= 0 && gray < 0.5) {
			g = gauss(gray, -.70487, 1.57141, 0.51782, 0.54700);
		}
		else {
			g = circle(gray, -.97384, 0.96412, 1.96264, 0.17749);
		}
		b = gauss(gray, -.05837, 1.05992, 0.28797, 0.39754);
	}
}

EQNodeRenderer::EQNodeRenderer() {
	//m_viewer = new osgViewer::Viewer;
	m_root = new osg::Group;
	m_geom = new osg::Geometry;
	m_nodes = new osg::Vec3Array;
	m_colors = new osg::Vec4Array;
}

void EQNodeRenderer::addNode(const eqtm::SphericCoord& sc) {
	//eq::CartesianCoord cc = equ::spheric_to_cartesian(sc);
	//m_nodes->push_back(osg::Vec3(cc.x, cc.y, cc.z));

	const osgEarth::SpatialReference* wgs84 = osgEarth::SpatialReference::get("wgs84", "egm96")->getGeographicSRS();
	const osgEarth::SpatialReference* ecef = wgs84->getECEF();

	osgEarth::GeoPoint p(wgs84, sc.longitude, sc.latitude, osgEarth::ALTMODE_ABSOLUTE);
	osgEarth::GeoPoint pc = p.transform(ecef);
	m_nodes->push_back(osg::Vec3(pc.x(), pc.y(), pc.z()));
}

void EQNodeRenderer::addAttr(const float& attr) {
	float r = 0.f, g = 0.f, b = 0.f;
	gray_to_rgb(attr, r, g, b);
	m_colors->push_back(osg::Vec4(r, g, b, 1.f));
}

void EQNodeRenderer::render() {
	//m_viewer->apply(new osgViewer::SingleScreen(0));
	//m_viewer->getCamera()->setClearColor(osg::Vec4(1, 1, 1, 1));

	m_geom->setVertexArray(m_nodes.get());
	m_geom->setColorArray(m_colors.get());
	m_geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, m_nodes->size()));
	m_geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

	osg::ref_ptr<osg::Point> pt = new osg::Point;
	pt->setSize(8);
	m_geom->getOrCreateStateSet()->setAttributeAndModes
		(pt.get(), osg::StateAttribute::ON);

	//osg::ref_ptr<osg::Shape> shape = new osg::Sphere(osg::Vec3(0, 0, 0), EQ_RADIUS * 0.995);
	//osg::ref_ptr<osg::ShapeDrawable> shape_drawable = new osg::ShapeDrawable(shape);
	//shape_drawable->setColor(osg::Vec4(0, 0, 0, 1));

	// ReSharper disable once CppNonReclaimedResourceAcquisition
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(m_geom.get());
	//geode->addDrawable(shape_drawable.get());
	geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	m_root->addChild(geode.get());

	//m_viewer->addEventHandler(new osgGA::StateSetManipulator(m_viewer->getCamera()->getOrCreateStateSet()));
	//m_viewer->addEventHandler(new osgViewer::WindowSizeHandler);
	//m_viewer->addEventHandler(new osgViewer::StatsHandler);
	//m_viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);

	//m_viewer->setSceneData(m_root.get());
	//m_viewer->realize();
	//m_viewer->run();
}

osg::Group* EQNodeRenderer::getNode(){
	return m_root.get();
}