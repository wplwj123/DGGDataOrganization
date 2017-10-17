#include "PickText.h"

#include <string>
#include <osgViewer/Viewer>
#include <osgText/Text>
#include <osg/ShapeDrawable>
#include <osgEarth/GeoData>
#include <osgEarthUtil/LatLongFormatter>

class UpdateTextCallback :public osg::Drawable::UpdateCallback{
public:
	UpdateTextCallback(osgText::Text* text = nullptr) :m_text(text){}

	void update(osg::NodeVisitor* nv, osg::Drawable* drawable) override {
		osgText::Text* t = static_cast<osgText::Text*>(drawable);
		if (t){
			t->setText(m_text->getText());
		}
	}
private:
	osg::ref_ptr<osgText::Text> m_text;
};


PickText::PickText(){
	lonText = new osgText::Text;
	latText = new osgText::Text;
	disText = new osgText::Text;
	lonText->setDataVariance(osg::Object::DYNAMIC);
	latText->setDataVariance(osg::Object::DYNAMIC);
	disText->setDataVariance(osg::Object::DYNAMIC);
}

osg::ref_ptr<osg::Camera> PickText::CreateLegend(int width, int height){
	//create legend
	osg::ref_ptr<osg::Camera> text_cam = new osg::Camera;
	text_cam->setProjectionMatrix(osg::Matrixd::ortho2D(0, width, 0, height));
	text_cam->setReferenceFrame(osg::Transform::ABSOLUTE_RF); // absolute frame refrence
	text_cam->setViewMatrix(osg::Matrix::identity());
	text_cam->setClearMask(GL_DEPTH_BUFFER_BIT); // clear buffer
	text_cam->setRenderOrder(osg::Camera::POST_RENDER);
	text_cam->setAllowEventFocus(false);

	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	osg::ref_ptr<osg::StateSet> stateset = geode->getOrCreateStateSet();
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

	lonText = CreateText(L"Longitude:-", width - 250, 100);
	lonText->setUpdateCallback(new UpdateTextCallback(lonText.get()));
	geode->addDrawable(lonText);
	latText = CreateText(L"Latitude:-", width - 250, 70);
	latText->setUpdateCallback(new UpdateTextCallback(latText.get()));
	geode->addDrawable(latText);
	disText = CreateText(L"Distance:-", width - 250, 40);
	disText->setUpdateCallback(new UpdateTextCallback(disText.get()));
	geode->addDrawable(disText);

	text_cam->addChild(geode.get());
	
	return text_cam;
}

osg::ref_ptr<osgText::Text> PickText::CreateText(const std::wstring& t, int x, int y){
	osg::ref_ptr<osgText::Font> font = osgText::readFontFile("simhei.ttf");
	osg::ref_ptr<osgText::Text> text = new osgText::Text;
	text->setFont(font.get());
	text->setText(t._Myptr());
	text->setPosition(osg::Vec3(x, y, 0));
	text->setCharacterSize(24.f);
	text->setFontResolution(60, 60);
	text->setColor(osg::Vec4(1.f, 1.f, 1.f, 1.f));
	text->setDrawMode(osgText::Text::TEXT);

	return text;
}

void PickText::UpdataLegend(const osgEarth::GeoPoint& pickPoint, const double distance){
	if (!pickPoint.isValid())
		return;

	static osgEarth::Util::LatLongFormatter s_f;

	lonText->setText(osgEarth::Stringify()
		<< std::fixed << std::setprecision(2) << "Longitude: "
		<< s_f.format(pickPoint.x(), true));
	latText->setText(osgEarth::Stringify()
		<< std::fixed << std::setprecision(2) << "Latitude: "
		<< s_f.format(pickPoint.y(), true));

	std::stringstream ss;
	ss << std::setprecision(10) << "Distance: " << distance;
	disText->setText(ss.str());
}

PickText::~PickText(){

}

