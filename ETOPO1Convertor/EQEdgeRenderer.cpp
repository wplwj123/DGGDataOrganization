#include "EQEdgeRenderer.h"

#include "../LibEQCommon/EQType.h"
#include "../LibEQUtil/TypeUtil.h"
#include "../LibEQUtil/GeoUtil.h"

#include <osg/ShapeDrawable>
#include <osgGA/StateSetManipulator>
#include <osgUtil/Optimizer>
#include <osgViewer/config/SingleScreen>
#include <osgViewer/ViewerEventHandlers>

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

EQEdgeRenderer::EQEdgeRenderer() {
	//m_viewer = new osgViewer::Viewer;
	m_root = new osg::Group;
	m_geom = new osg::Geometry;
	m_nodes = new osg::Vec3Array;
	m_colors = new osg::Vec4Array;
}

void EQEdgeRenderer::addNode(const eqtm::SphericCoord& sc) {
	//eq::CartesianCoord cc = equ::spheric_to_cartesian(sc);
	//m_nodes->push_back(osg::Vec3(cc.x, cc.y, cc.z));

	const osgEarth::SpatialReference* wgs84 = osgEarth::SpatialReference::get("wgs84", "egm96")->getGeographicSRS();
	const osgEarth::SpatialReference* ecef = wgs84->getECEF();

	osgEarth::GeoPoint p(wgs84, sc.longitude, sc.latitude, osgEarth::ALTMODE_ABSOLUTE);
	osgEarth::GeoPoint pc = p.transform(ecef);
	m_nodes->push_back(osg::Vec3(pc.x(), pc.y(), pc.z()));
}

void EQEdgeRenderer::addAttr(const float& attr) {
	float r = 0.f, g = 0.f, b = 0.f;
	gray_to_rgb(attr, r, g, b);
	m_colors->push_back(osg::Vec4(r, g, b, 1.f));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void EQEdgeRenderer::addIndices(const int& v0, const int& v1) {
	osg::ref_ptr<osg::DrawElementsUInt> edge =
		new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, 0);
	edge->push_back(v0);
	edge->push_back(v1);
	m_geom->addPrimitiveSet(edge.get());
}

void EQEdgeRenderer::render() {
	//m_viewer->apply(new osgViewer::SingleScreen(0));
	//m_viewer->getCamera()->setClearColor(osg::Vec4(1, 1, 1, 1));

	m_geom->setVertexArray(m_nodes.get());
	m_geom->setColorArray(m_colors.get());
	m_geom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);

	// ReSharper disable once CppNonReclaimedResourceAcquisition
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(m_geom.get());
#if 0
	{
		osg::ref_ptr<osg::Sphere> s = new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), EQ_RADIUS - 10);
		osg::ref_ptr<osg::ShapeDrawable> shape = new osg::ShapeDrawable;
		shape->setShape(s.get());
		shape->setColor(osg::Vec4(1, 1, 1, 1));
		geode->addDrawable(shape);
	}
#endif
	geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	m_root->addChild(geode.get());

	osgUtil::Optimizer op;
	op.optimize(m_root.get());

	//m_viewer->addEventHandler(new osgGA::StateSetManipulator(m_viewer->getCamera()->getOrCreateStateSet()));
	//m_viewer->addEventHandler(new osgViewer::WindowSizeHandler);
	//m_viewer->addEventHandler(new osgViewer::StatsHandler);
	//m_viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);

	//m_viewer->setSceneData(m_root.get());
	//m_viewer->realize();
	//m_viewer->run();
}

osg::Group* EQEdgeRenderer::getNode(){
	return m_root.get();
}