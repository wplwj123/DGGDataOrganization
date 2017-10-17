#include "IRasterRenderer.h"

#include <osgViewer/Viewer>

class EQCellRenderer :public IRasterRenderer {
public:
	EQCellRenderer();

	void addNode(const eqtm::SphericCoord& sc) override;
	osg::Group* getNode() override;
	void addAttr(const float& attr) override;
	void addIndices(const int& v0, const int& v1, const int& v2);
	void render() override;

private:
	//osg::ref_ptr<osgViewer::Viewer> m_viewer;
	osg::ref_ptr<osg::Group> m_root;
	osg::ref_ptr<osg::Geometry> m_geom;

	osg::ref_ptr<osg::Vec3Array> m_nodes;
	osg::ref_ptr<osg::Vec4Array> m_colors;
};