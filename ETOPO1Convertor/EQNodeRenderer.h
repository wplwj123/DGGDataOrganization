#include "IRasterRenderer.h"

#include <osgViewer/Viewer>

class EQNodeRenderer :public IRasterRenderer {
public:
	EQNodeRenderer();

	void addNode(const eqtm::SphericCoord& sc) override;
	void addAttr(const float& attr) override;
	osg::Group* getNode() override;
	void render() override;

private:
	//osg::ref_ptr<osgViewer::Viewer> m_viewer;
	osg::ref_ptr<osg::Group> m_root;
	osg::ref_ptr<osg::Geometry> m_geom;

	osg::ref_ptr<osg::Vec3Array> m_nodes;
	osg::ref_ptr<osg::Vec4Array> m_colors;
};