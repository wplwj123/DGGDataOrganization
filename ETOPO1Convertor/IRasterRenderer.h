#pragma once

#include <osg/Group>

namespace eqtm {
	struct SphericCoord;
}

class IRasterRenderer {
public:
	virtual ~IRasterRenderer() {
	}

	virtual void addNode(const eqtm::SphericCoord& sc) = 0;
	virtual void addAttr(const float& attr) = 0;

	virtual osg::Group* getNode() = 0;

	virtual void render() = 0;
};
