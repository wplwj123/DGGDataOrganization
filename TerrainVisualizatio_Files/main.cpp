
#include <iostream>
#include <string>

#include <osg/PagedLOD>
#include <osg/ClusterCullingCallback>
#include <osgViewer/Viewer>
#include <osgViewer/config/SingleScreen>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osgUtil/Optimizer>

#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>
#include <osgEarthDrivers/feature_ogr/OGRFeatureOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>

#include <tclap/CmdLine.h>

#include "../LibEQCommon/EQType.h"
#include "../LibEQCommon/EQCodec.h"
#include "../LibEQUtil/TypeUtil.h"
#include "../LibEQUtil/GeoUtil.h"

#if _DEBUG
#pragma comment(lib, "osgd.lib")
#pragma comment(lib, "osgDBd.lib")
#pragma comment(lib, "osgGAd.lib")
#pragma comment(lib, "osgViewerd.lib")
#pragma comment(lib, "osgUtild.lib")
#pragma comment(lib, "osgEarthd.lib")
#pragma comment(lib, "osgEarthUtild.lib")
#pragma comment(lib, "osgEarthFeaturesd.lib")
#pragma comment(lib, "osgEarthSymbologyd.lib")
#pragma comment(lib, "../build/LibEQCommond.lib")
#pragma comment(lib, "../build/LibEQUtild.lib")
#else
#pragma comment(lib, "osg.lib")
#pragma comment(lib, "osgDB.lib")
#pragma comment(lib, "osgGA.lib")
#pragma comment(lib, "osgViewer.lib")
#pragma comment(lib, "osgUtil.lib")
#pragma comment(lib, "osgEarth.lib")
#pragma comment(lib, "osgEarthUtil.lib")
#pragma comment(lib, "osgEarthFeatures.lib")
#pragma comment(lib, "osgEarthSymbology.lib")
#pragma comment(lib, "../build/LibEQCommon.lib")
#pragma comment(lib, "../build/LibEQUtil.lib")
#endif

const osgEarth::SpatialReference* wgs84 = osgEarth::SpatialReference::get("wgs84", "egm96")->getGeographicSRS();
const osgEarth::SpatialReference* ecef = wgs84->getECEF();

osg::Vec3 SpCoord2ECEF(const eqtm::SphericCoord& sc){

	osgEarth::GeoPoint p(wgs84, sc.longitude, sc.latitude, osgEarth::ALTMODE_ABSOLUTE);
	osgEarth::GeoPoint pc = p.transform(ecef);

	return osg::Vec3(pc.x(), pc.y(), pc.z());
}


int main(int argc, char** argv){
	
	std::string workDirectory;
	unsigned int minLevel, maxLevel;

	try{
		TCLAP::CmdLine cmd("Command description message", ' ', "0.1");
		TCLAP::ValueArg<std::string> workDirArg("d", "dir", "Pyramid folder path", true, "homer", "string");
		cmd.add(workDirArg);
		TCLAP::ValueArg<unsigned int> minLevelArg("i", "minlevel", "min grid level", true, 0, "int");
		cmd.add(minLevelArg);
		TCLAP::ValueArg<unsigned int> maxLevelArg("a", "maxlevel", "max grid level", true, 5, "int");
		cmd.add(maxLevelArg);
		cmd.parse(argc, argv);
		
		workDirectory = workDirArg.getValue();
		minLevel = minLevelArg.getValue();
		maxLevel = maxLevelArg.getValue();
	}
	catch (TCLAP::ArgException& e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		return EXIT_FAILURE;
	}

	osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
	viewer->apply(new osgViewer::SingleScreen(0));
	viewer->setThreadingModel(osgViewer::ViewerBase::CullThreadPerCameraDrawThreadPerContext);
	viewer->getCamera()->setClearColor(osg::Vec4(0, 0, 0, 1));

	osg::ref_ptr<osg::Group> root = new osg::Group;

	osg::ref_ptr<osgEarth::Map> map = new osgEarth::Map;
	osg::ref_ptr<osgEarth::MapNode> mapNode = new osgEarth::MapNode(map);
	root->addChild(mapNode);

	//add sky layer
	osg::ref_ptr<osgEarth::Util::SkyNode> skyNode = osgEarth::Util::SkyNode::create(mapNode);
	skyNode->setName("SkyNode");
	osgEarth::DateTime dateTime(2016, 11, 11, 16);
	osg::ref_ptr<osgEarth::Util::Ephemeris> ephemeris = new osgEarth::Util::Ephemeris;
	skyNode->setEphemeris(ephemeris);
	skyNode->setDateTime(dateTime);
	skyNode->attach(viewer, 0);
	skyNode->setLighting(false);
	root->addChild(skyNode);

	osg::ref_ptr<osg::Group> dggNode = new osg::Group;
	root->addChild(dggNode);

	const float viewDistance = 10000000;
	for (unsigned int level = minLevel; level <= maxLevel; level++){
		for (unsigned int dom_id = 0; dom_id < 10; dom_id++){
			eqtm::EQCode areaCode;
			areaCode.dt = 0x04 + static_cast<int>(dom_id << 4);
			areaCode.len = level;
			areaCode.morton = 0;

			EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

			for (EQ_ULLONG mor_id = 0; mor_id < n_cell_in_domain; ++mor_id) {
				areaCode.morton = mor_id;

				//calculate the center and normal of the tile
				EQ_UINT code_domain = 0;
				EQ_UINT code_level = 0;
				eqtm::Trigon tri = eqtm::decode(areaCode, code_domain, code_level);
				eqtm::CartesianCoord v1 = eqtm::util::spheric_to_cartesian(tri.v(1));
				eqtm::CartesianCoord v3 = eqtm::util::spheric_to_cartesian(tri.v(2));
				eqtm::SphericCoord centerSC = eqtm::util::cartesian_to_spheric(eqtm::util::mid_great_arc(v1, v3));

				osg::Vec3 center = SpCoord2ECEF(centerSC);
				osg::Vec3 normal = wgs84->getEllipsoid()->computeLocalUpVector(center.x(), center.y(), center.z());

				//get the filePath by code
				std::stringstream ss;
				//ss << workDirectory << "\\" << "Level-" << level << "\\" << dom_id << "-" << mor_id << ".ive";
				ss << workDirectory << "\\" << "Level-" << level << "\\" << areaCode.dt << "-" << mor_id << ".ive";
				std::string filePath;
				ss >> filePath;

				osg::ref_ptr<osg::PagedLOD> pageNode = new osg::PagedLOD;
				pageNode->setCullCallback(new osg::ClusterCullingCallback(center, normal, 0.0));
				pageNode->setCenter(center);
				pageNode->setFileName(0, filePath);

				//set range 
				if (level == 5){
					if (dom_id == 1){
						pageNode->setRange(0, viewDistance / pow(2, level), viewDistance / pow(2, level - 1));
					}
					else{
						pageNode->setRange(0, FLT_MIN, viewDistance / pow(2, level - 1));
					}
				}
				else if (level == 0){
					pageNode->setRange(0, viewDistance / pow(2, level), FLT_MAX);
				}
				else{
					pageNode->setRange(0, viewDistance / pow(2, level), viewDistance / pow(2, level - 1));
				}

				dggNode->addChild(pageNode);
			}

		}
	}

	//grid data from China SRTM (6 to 8)
	for (unsigned int level = 6; level <= 8; level++){
		for (unsigned int dom_id = 1; dom_id < 2; dom_id++){
			eqtm::EQCode areaCode;
			areaCode.dt = 0x04 + static_cast<int>(dom_id << 4);
			areaCode.len = level;
			areaCode.morton = 0;

			EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

			for (EQ_ULLONG mor_id = 0; mor_id < n_cell_in_domain; ++mor_id){
				areaCode.morton = mor_id;

				EQ_UINT code_domain = 0;
				EQ_UINT code_level = 0;
				eqtm::Trigon tri = eqtm::decode(areaCode, code_domain, code_level);
				eqtm::CartesianCoord v1 = eqtm::util::spheric_to_cartesian(tri.v(1));
				eqtm::CartesianCoord v3 = eqtm::util::spheric_to_cartesian(tri.v(2));
				eqtm::SphericCoord centerSC = eqtm::util::cartesian_to_spheric(eqtm::util::mid_great_arc(v1, v3));

				osg::Vec3 center = SpCoord2ECEF(centerSC);
				osg::Vec3 normal = wgs84->getEllipsoid()->computeLocalUpVector(center.x(), center.y(), center.z());

				//get the filePath by code
				std::stringstream ss;
				//ss << workDirectory << "\\" << "Level-" << level << "\\" << dom_id << "-" << mor_id << ".ive";
				ss << workDirectory << "\\" << "Level-" << level << "\\" << areaCode.dt << "-" << mor_id << ".ive";
				std::string filePath;
				ss >> filePath;

				osg::ref_ptr<osg::PagedLOD> pageNode = new osg::PagedLOD;
				pageNode->setCullCallback(new osg::ClusterCullingCallback(center, normal, 0.0));
				pageNode->setCenter(center);
				pageNode->setFileName(0, filePath);

				if (level == 8){
					pageNode->setRange(0, FLT_MIN, viewDistance / pow(2, level - 1));
				}
				else{
					pageNode->setRange(0, viewDistance / pow(2, level), viewDistance / pow(2, level - 1));
				}

				dggNode->addChild(pageNode);
			}
		}
	}

	//add country boundary shp layer
	osgEarth::Drivers::OGRFeatureOptions shplayerOpt;
	shplayerOpt.url() = osgEarth::URI("E:\\OSG\\osgEarth\\data\\world.shp");
	//set shp style
	osgEarth::Symbology::Style style;
	style.setName("shpStyle");
	osg::ref_ptr<osgEarth::LineSymbol> ls = style.getOrCreateSymbol<osgEarth::LineSymbol>();
	ls->stroke()->color() = osgEarth::Color::Gray;
	ls->stroke()->width() = 4.0f;
	style.getOrCreate<osgEarth::AltitudeSymbol>()->clamping() = osgEarth::AltitudeSymbol::CLAMP_RELATIVE_TO_TERRAIN;
	style.getOrCreate<osgEarth::AltitudeSymbol>()->technique() = osgEarth::AltitudeSymbol::TECHNIQUE_DRAPE;
	style.getOrCreate<osgEarth::AltitudeSymbol>()->binding() = osgEarth::AltitudeSymbol::BINDING_VERTEX;
	//create shp layer
	osgEarth::Drivers::FeatureGeomModelOptions geomOptions;
	geomOptions.featureOptions() = shplayerOpt;
	geomOptions.styles() = new osgEarth::StyleSheet();
	geomOptions.styles()->addStyle(style);
	geomOptions.enableLighting() = false;
	osg::ref_ptr<osgEarth::ModelLayer> shpLayer = new osgEarth::ModelLayer("shp", geomOptions);
	map->addModelLayer(shpLayer);
	map->moveModelLayer(shpLayer, 0);

	viewer->setUpViewInWindow(50, 50, 1400, 800);

	osg::ref_ptr<osgEarth::Util::EarthManipulator> manip = new osgEarth::Util::EarthManipulator();
	manip->getSettings()->setArcViewpointTransitions(true);
	manip->setHomeViewpoint(osgEarth::Viewpoint("World", 110.0, 30.0, 0.0, 0.0, -90.0, (10e6) * 2));
	viewer->setCameraManipulator(manip);

	viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
	viewer->addEventHandler(new osgViewer::LODScaleHandler);
	viewer->addEventHandler(new osgViewer::ThreadingHandler);
	viewer->addEventHandler(new osgViewer::WindowSizeHandler);
	viewer->addEventHandler(new osgViewer::StatsHandler);
	viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);

	viewer->getCamera()->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
	viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES);
	viewer->getCamera()->setNearFarRatio(0.00001f);

	//viewer->getCamera()->setCullingMode(osg::CullSettings::VIEW_FRUSTUM_CULLING);
	//viewer->getCamera()->setCullingMode(osg::CullSettings::VIEW_FRUSTUM_SIDES_CULLING);
	viewer->getCamera()->setCullingMode(osg::CullSettings::CLUSTER_CULLING | osg::CullSettings::VIEW_FRUSTUM_SIDES_CULLING);
	//viewer->getCamera()->setCullingMode(osg::CullSettings::SHADOW_OCCLUSION_CULLING | osg::CullSettings::CLUSTER_CULLING);

	osgUtil::Optimizer optimizer;
	optimizer.optimize(root);

	viewer->getDatabasePager()->setTargetMaximumNumberOfPageLOD(100);
	viewer->setSceneData(root);
	viewer->realize();
	viewer->run();
	return 0;
}