
#include <iostream>
#include <string>
#include <vector>

#include <osg/PagedLOD>
#include <osg/ClusterCullingCallback>
#include <osg/PositionAttitudeTransform>
#include <osgViewer/Viewer>
#include <osgViewer/config/SingleScreen>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/Optimizer>

#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>
#include <osgEarthDrivers/feature_ogr/OGRFeatureOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>

#include <tclap/CmdLine.h>
#include <winsock2.h> 
#include <mongo/client/dbclient.h> 
#include <hiredis.h>

#include "../LibEQCommon/EQType.h"
#include "../LibEQCommon/EQCodec.h"
#include "../LibEQUtil/TypeUtil.h"
#include "../LibEQUtil/GeoUtil.h"

#include "PickHandler.h"
#include "PickText.h"

#if _DEBUG
#pragma comment(lib, "libboost_system-vc120-mt-gd-1_64")
#pragma comment(lib, "libboost_chrono-vc120-mt-gd-1_64")
#pragma comment(lib, "libboost_date_time-vc120-mt-gd-1_64")
#pragma comment(lib, "libboost_thread-vc120-mt-gd-1_64")
#pragma comment(lib, "mongoclient-gd")
#pragma comment(lib, "osgd.lib")
#pragma comment(lib, "osgTextd.lib")
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
#pragma comment(lib, "hiredisd.lib")
#pragma comment(lib, "Win32_Interopd.lib")
#else
#pragma comment(lib, "libboost_system-vc120-mt-1_64")
#pragma comment(lib, "libboost_chrono-vc120-mt-1_64")
#pragma comment(lib, "libboost_date_time-vc120-mt-1_64")
#pragma comment(lib, "libboost_thread-vc120-mt-1_64")
#pragma comment(lib, "mongoclient")
#pragma comment(lib, "osg.lib")
#pragma comment(lib, "osgText.lib")
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
#pragma comment(lib, "hiredis.lib")
#pragma comment(lib, "Win32_Interop.lib")
#endif

std::string dbName;

int redisNum = 0;
int totelNum = 0;

mongo::DBClientConnection mongoConn;
redisContext* redisConn;
redisReply* reply;

const float viewDistance = 10000000;

const osgEarth::SpatialReference* wgs84 = osgEarth::SpatialReference::get("wgs84", "egm96")->getGeographicSRS();
const osgEarth::SpatialReference* ecef = wgs84->getECEF();
osg::Vec3 SpCoord2ECEF(const eqtm::SphericCoord& sc){

	osgEarth::GeoPoint p(wgs84, sc.longitude, sc.latitude, osgEarth::ALTMODE_ABSOLUTE);
	osgEarth::GeoPoint pc = p.transform(ecef);

	return osg::Vec3(pc.x(), pc.y(), pc.z());
}

class NodeBuf : public std::basic_streambuf<char, std::char_traits<char>> {

public:
	//构造函数，参数意义：buffer的地址和长度
	NodeBuf(char* mem, int sz) {
		//参数意义：起始位置，当前位置，最后的位置
		setg(mem, mem, mem + sz);
	}
};

//get  tile from MongoDB
class MongoDBCallBack : public osgDB::ReadFileCallback{

public:
	virtual osgDB::ReaderWriter::ReadResult readNode(const std::string &fileName, const osgDB::ReaderWriter::Options *options){

		int firstIndex = fileName.find_first_of("-");
		int lastIndex = fileName.find_last_of("-");

		if (firstIndex == lastIndex){
			return nullptr;
		}

		std::stringstream ss;
		ss << fileName.substr(0, firstIndex);
		unsigned int level;
		ss >> level;

		ss.str("");
		ss.clear();
		ss << fileName.substr(firstIndex + 1, lastIndex);
		unsigned int domID;
		ss >> domID;

		ss.str("");
		ss.clear();
		ss << fileName.substr(lastIndex + 1);
		unsigned long long morID;
		ss >> morID;

		unsigned int smallLevel = level % 3;
		unsigned int pyramidLevel = level - smallLevel;

		ss.str("");
		ss.clear();
		ss << dbName << ".Level" << pyramidLevel;
		std::string collName = ss.str();

		mongo::BSONObj topTile = mongoConn.findOne(collName, mongo::BSONObjBuilder().append("DomainID", domID).append("MortonCode", static_cast<long long>(morID >> (2 * smallLevel))).obj());

		char* content = nullptr;
		int size = 0;
		if (topTile.isValid()){
			if (smallLevel == 0){      //top level
				content = const_cast<char*>(topTile.getField("Content").binData(size));
			}
			else if (smallLevel == 1){        //middle level
				bool isNull = true;
				for each (auto iter in topTile.getField("SubPyramidTile").Array())
				{
					if (morID == iter.Obj().getField("MortonCode").Long()){
						isNull = false;
						content = const_cast<char*>(iter.Obj().getField("Content").binData(size));
						break;
					}
				}
				if (isNull == true){
					return nullptr;
				}
			}
			else{                      //buttom level
				mongo::BSONElement midTile;
				bool isNull = true;
				for each (auto iter in topTile.getField("SubPyramidTile").Array())
				{
					if ((morID >> 2) == iter.Obj().getField("MortonCode").Long()){
						isNull = false;
						midTile = iter;
						break;
					}
				}
				if (isNull == true){
					return nullptr;
				}
				isNull = true;
				for each (auto iter in midTile.Obj().getField("SubPyramidTile").Array())
				{
					if (morID == iter.Obj().getField("MortonCode").Long()){
						isNull = false;
						content = const_cast<char*>(iter.Obj().getField("Content").binData(size));
						break;
					}
				}
				if (isNull == true){
					return nullptr;
				}
			}
		}
		else{
			return nullptr;
		}

		if (content == nullptr){
			return nullptr;
		}

		std::cout << fileName << " from MongoDB: " << redisNum << "/" << (++totelNum) << std::endl;

		NodeBuf buffer(content, size);
		std::istream stream(&buffer);

		osg::ref_ptr<osgDB::ReaderWriter> readWriter = osgDB::Registry::instance()->getReaderWriterForExtension("ive");
		osgDB::ReaderWriter::ReadResult result = readWriter->readNode(stream);

		return result;
	}
};

//get tile from Redis and MongoDB
class TileReadFileCallBack : public osgDB::ReadFileCallback{

public:
	virtual osgDB::ReaderWriter::ReadResult readNode(const std::string &fileName, const osgDB::ReaderWriter::Options *options){

		int firstIndex = fileName.find_first_of("-");
		int lastIndex = fileName.find_last_of("-");

		if (firstIndex == lastIndex){
			return nullptr;
		}

		reply = (redisReply*)redisCommand(redisConn, "GET %s", fileName.c_str());

		std::string contStr = "";
		int size = 0;

		if (reply->type == REDIS_REPLY_STRING){     //get tile from Redis
			std::cout << fileName << " from Redis:" << (++redisNum) << "/" << (++totelNum) << std::endl;

			contStr = std::string(reply->str, reply->len);
			size = reply->len;
			freeReplyObject(reply);
		}
		else if (reply->type == REDIS_REPLY_NIL){   //get tile from MongoDB and save to Redis
			std::cout << fileName << " from MongoDB: " << redisNum << "/" << (++totelNum) << std::endl;

			std::stringstream ss;
			ss << fileName.substr(0, firstIndex);
			unsigned int level;
			ss >> level;

			ss.str("");
			ss.clear();
			ss << fileName.substr(firstIndex + 1, lastIndex);
			unsigned int domID;
			ss >> domID;

			ss.str("");
			ss.clear();
			ss << fileName.substr(lastIndex + 1);
			unsigned long long morID;
			ss >> morID;

			unsigned int smallLevel = level % 3;
			unsigned int pyramidLevel = level - smallLevel;

			ss.str("");
			ss.clear();
			ss << dbName << ".Level" << pyramidLevel;
			std::string collName = ss.str();

			mongo::BSONObj topTile = mongoConn.findOne(collName, mongo::BSONObjBuilder().append("DomainID", domID).append("MortonCode", static_cast<long long>(morID >> (2 * smallLevel))).obj());

			char* content = nullptr;
			if (topTile.isValid()){
				if (smallLevel == 0){      //top level
					content = const_cast<char*>(topTile.getField("Content").binData(size));
				}
				else if (smallLevel == 1){        //middle level
					bool isNull = true;
					for each (auto iter in topTile.getField("SubPyramidTile").Array())
					{
						if (morID == iter.Obj().getField("MortonCode").Long()){
							isNull = false;
							content = const_cast<char*>(iter.Obj().getField("Content").binData(size));
							break;
						}
					}
					if (isNull == true){
						return nullptr;
					}
				}
				else{                      //buttom level
					mongo::BSONElement midTile;
					bool isNull = true;
					for each (auto iter in topTile.getField("SubPyramidTile").Array())
					{
						if ((morID >> 2) == iter.Obj().getField("MortonCode").Long()){
							isNull = false;
							midTile = iter;
							break;
						}
					}
					if (isNull == true){
						return nullptr;
					}
					isNull = true;
					for each (auto iter in midTile.Obj().getField("SubPyramidTile").Array())
					{
						if (morID == iter.Obj().getField("MortonCode").Long()){
							isNull = false;
							content = const_cast<char*>(iter.Obj().getField("Content").binData(size));
							break;
						}
					}
					if (isNull == true){
						return nullptr;
					}
				}
			}
			else{
				return nullptr;
			}

			if (content != nullptr && size != 0){
				freeReplyObject(reply);
				reply = (redisReply*)redisCommand(redisConn, "SET %s %b", fileName.c_str(), content, size);
			}
			freeReplyObject(reply);

			contStr = std::string(content, size);

		}
		else{
			std::cout << "search " << fileName << " faile:" << redisConn->errstr << std::endl;
			freeReplyObject(reply);
			return nullptr;
		}

		char* contIve = const_cast<char*>(contStr.c_str());
		NodeBuf buffer(contIve, size);
		std::istream stream(&buffer);

		osg::ref_ptr<osgDB::ReaderWriter> readWriter = osgDB::Registry::instance()->getReaderWriterForExtension("ive");
		osgDB::ReaderWriter::ReadResult result = readWriter->readNode(stream);

		return result;
	}
};

int main(int argc, char** argv){

	std::string url;

	try{
		TCLAP::CmdLine cmd("Command description message", ' ', "0.1");
		TCLAP::ValueArg<std::string> dbNameArg("n", "db", "Database Name", true, "homer", "string");
		cmd.add(dbNameArg);
		TCLAP::ValueArg<std::string> urlArg("u", "url", "url", true, "homer", "string");
		cmd.add(urlArg);
		cmd.parse(argc, argv);

		dbName = dbNameArg.getValue();
		url = urlArg.getValue();
	}
	catch (TCLAP::ArgException& e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		return EXIT_FAILURE;
	}

	//connect to MongoDB
	//Local Url: 127.0.0.1
	//Network Url: 192.168.1.101

	mongo::client::initialize();
	try {
		mongoConn.connect(url + ":27017");
		std::cout << "Connect to MongoDB Server Success" << std::endl;
	}
	catch (const mongo::DBException &e) {
		std::cout << "caught " << e.what() << std::endl;
		return -1;
	}

	//connect to Redis
	redisConn = redisConnect(url.c_str(), 6379);

	if (redisConn->err)
	{
		std::cout << "Connect to Redis Server faile:" << redisConn->errstr << std::endl;
		redisFree(redisConn);
	}
	std::cout << "Connect to Redis Server Success" << std::endl;


	osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer();
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

	//add dggNode
	osg::ref_ptr<osg::Group> dggNode = new osg::Group;
	mapNode->addChild(dggNode);
	osgDB::Registry::instance()->setReadFileCallback(new TileReadFileCallBack());
	//osgDB::Registry::instance()->setReadFileCallback(new MongoDBCallBack());

	//grid data from ETOPO1 (0 to 5)
	for (unsigned int level = 0; level <= 5; level++){
		for (unsigned int dom_id = 0; dom_id < 10; dom_id++){
			eqtm::EQCode areaCode;
			areaCode.dt = 0x04 + static_cast<int>(dom_id << 4);
			areaCode.len = level;
			areaCode.morton = 0;

			EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

			for (EQ_ULLONG mor_id = 0; mor_id < n_cell_in_domain; ++mor_id){
				areaCode.morton = mor_id;

				//calculate canter and normal of the tile
				EQ_UINT code_domain = 0;
				EQ_UINT code_level = 0;
				eqtm::Trigon tri = eqtm::decode(areaCode, code_domain, code_level);
				eqtm::CartesianCoord v1 = eqtm::util::spheric_to_cartesian(tri.v(1));
				eqtm::CartesianCoord v3 = eqtm::util::spheric_to_cartesian(tri.v(2));
				eqtm::SphericCoord centerSC = eqtm::util::cartesian_to_spheric(eqtm::util::mid_great_arc(v1, v3));

				osg::Vec3 center = SpCoord2ECEF(centerSC);
				osg::Vec3 normal = wgs84->getEllipsoid()->computeLocalUpVector(center.x(), center.y(), center.z());

				std::stringstream ss;
				ss << level << "-" << dom_id << "-" << mor_id;
				std::string fileName = ss.str();

				osg::ref_ptr<osg::PagedLOD> pageNode = new osg::PagedLOD;
				pageNode->setCullCallback(new osg::ClusterCullingCallback(center, normal, 0.0));    //set ClusterCulling callback
				pageNode->setCenter(center);
				pageNode->setFileName(0, fileName);

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

				std::stringstream ss;
				ss << level << "-" << dom_id << "-" << mor_id;
				std::string fileName = ss.str();

				osg::ref_ptr<osg::PagedLOD> pageNode = new osg::PagedLOD;
				pageNode->setCullCallback(new osg::ClusterCullingCallback(center, normal, 0.0));
				pageNode->setCenter(center);
				pageNode->setFileName(0, fileName);

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
	viewer->addEventHandler(new osgViewer::RecordCameraPathHandler);

	viewer->getCamera()->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
	viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES);
	viewer->getCamera()->setNearFarRatio(0.00001f);

	//set Culling Mode
	//viewer->getCamera()->setCullingMode(osg::CullSettings::VIEW_FRUSTUM_CULLING);
	viewer->getCamera()->setCullingMode(osg::CullSettings::VIEW_FRUSTUM_SIDES_CULLING);
	//viewer->getCamera()->setCullingMode(osg::CullSettings::CLUSTER_CULLING | osg::CullSettings::VIEW_FRUSTUM_SIDES_CULLING);
	//viewer->getCamera()->setCullingMode(osg::CullSettings::SHADOW_OCCLUSION_CULLING | osg::CullSettings::CLUSTER_CULLING);

	viewer->setUpViewInWindow(50, 50, 1400, 800);

	//create lon/lat text
	PickText* pText = new PickText();
	root->addChild(pText->CreateLegend(viewer->getCamera()->getViewport()->width(), viewer->getCamera()->getViewport()->height()));
	viewer->addEventHandler(new PickHandler(pText));

	//print camera parameter
	double left, right, bottom, top, zNear, zFar;
	if (viewer->getCamera()->getProjectionMatrixAsFrustum(left, right, bottom, top, zNear, zFar)){
		std::cout << "\nLeft: " << left << ", Right: " << right << std::endl;
		std::cout << "Bottom: " << bottom << ", Top: " << top << std::endl;
		std::cout << "zNear: " << zNear << ", zFar: " << zFar << std::endl;
	}

	osgUtil::Optimizer optimizer;
	optimizer.optimize(root);
	viewer->setSceneData(root);

	viewer->getDatabasePager()->setTargetMaximumNumberOfPageLOD(100);  //set max number of pagelod
	viewer->realize();
	viewer->run();

	return 0;
}

