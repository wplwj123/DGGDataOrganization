
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <random>

#include <winsock2.h> 
#include <io.h>

#include <tclap/CmdLine.h>
#include <mongo/client/dbclient.h> // for the driver

#include "../LibEQCommon/EQType.h"
#include "../LibEQCommon/EQCodec.h"
#include "../LibEQUtil/TypeUtil.h"
#include "../LibEQUtil/GeoUtil.h"

#if _DEBUG
#pragma comment(lib, "libboost_system-vc120-mt-gd-1_64")
#pragma comment(lib, "libboost_chrono-vc120-mt-gd-1_64")
#pragma comment(lib, "libboost_date_time-vc120-mt-gd-1_64")
#pragma comment(lib, "libboost_thread-vc120-mt-gd-1_64")
#pragma comment(lib, "../build/LibEQCommond.lib")
#pragma comment(lib, "../build/LibEQUtild.lib")
#pragma comment(lib, "mongoclient-gd")
#pragma comment(lib, "winmm.lib")
#else
#pragma comment(lib, "libboost_system-vc120-mt-1_64")
#pragma comment(lib, "libboost_chrono-vc120-mt-1_64")
#pragma comment(lib, "libboost_date_time-vc120-mt-1_64")
#pragma comment(lib, "libboost_thread-vc120-mt-1_64")
#pragma comment(lib, "../build/LibEQCommon.lib")
#pragma comment(lib, "../build/LibEQUtil.lib")
#pragma comment(lib, "mongoclient")
#pragma comment(lib, "winmm.lib")
#endif


std::string getFilePath(const eqtm::EQCode& code, const std::string& dataDirectory){

	std::stringstream ssFilePath;
	ssFilePath << dataDirectory << "\\Level-" << code.len << "\\" << (code.dt >> 4) << "-" << code.morton << ".ive";

	return std::string(ssFilePath.str());
}

bool isExist(const std::string& filePath){

	bool haveFile = false;
	std::fstream _file;
	_file.open(filePath, std::ios::in);
	if (_file){
		haveFile = true;
	}
	_file.close();

	return haveFile;
}


void createDocument(mongo::BSONObjBuilder& bob, const eqtm::EQCode& code, const std::string& dataDirectory){

	std::string fileName = getFilePath(code, dataDirectory);

	std::size_t fileSize = 0;
	std::ifstream in(fileName, std::ios::binary);

	in.seekg(0, std::ios::end);
	fileSize = in.tellg();
	in.seekg(std::ios::beg);

	char* buffer = new char[fileSize + 1];
	in.read(buffer, fileSize);
	in.close();

	bob.genOID();
	bob.append("Level", code.len);
	bob.append("MortonCode", static_cast<long long>(code.morton));
	bob.append("DomainID", (code.dt >> 4));
	bob.append("ElementType", (code.dt & 12));
	bob.appendBinData("Content", fileSize, mongo::BinDataGeneral, buffer);

	delete[] buffer;
}

void createTile(mongo::BSONObjBuilder& bob, const eqtm::EQCode& code, const std::string& dataDirectory){

	createDocument(bob, code, dataDirectory);

	if (code.len % 3 != 2){    //not low level of small pyramid
		mongo::BSONArrayBuilder bab;
		for (unsigned int i = 0; i < 4; i++){
			eqtm::EQCode subCode;
			subCode.dt = code.dt;
			subCode.len = code.len + 1;
			subCode.morton = (code.morton << 2) + i;

			if (isExist(getFilePath(subCode, dataDirectory)) == false){
				continue;
			}

			mongo::BSONObjBuilder subBob;
			createTile(subBob, subCode, dataDirectory);
			bab.append(subBob.obj());
		}
		bob.appendArray("SubPyramidTile", bab.obj());
	}

}

int main(int argc, char** argv){
	
	std::string dataDirectory;
	std::string dbName;

	try{
		TCLAP::CmdLine cmd("Command description message", ' ', "0.1");
		TCLAP::ValueArg<std::string> dataDirArg("d", "dir", "Pyramid folder path", true, "homer", "string");
		cmd.add(dataDirArg);
		TCLAP::ValueArg<std::string> dbNameArg("n", "db", "Database Name", true, "homer", "string");
		cmd.add(dbNameArg);
		cmd.parse(argc, argv);

		dataDirectory = dataDirArg.getValue();
		dbName = dbNameArg.getValue();
	}
	catch (TCLAP::ArgException& e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		return EXIT_FAILURE;
	}
	
	mongo::client::initialize();
	mongo::DBClientConnection conn;
	try {
		conn.connect("127.0.0.1:27017");
		std::cout << "connected ok" << std::endl;
	}
	catch (const mongo::DBException &e) {
		std::cout << "caught " << e.what() << std::endl;
		return -1;
	}

#ifdef ETOPO1_INPUT

	for (unsigned int level = 0; level <= 5; level++){

		if (level % 3 == 0){               //top level of small Pyramid
			std::stringstream ssTableName;
			ssTableName << dbName << ".Level" << level;

			std::cout << "start creat " << ssTableName.str() << std::endl;

			std::string tableName = ssTableName.str();
			conn.createCollection(tableName);

			for (unsigned int domID = 0; domID < 10; domID++){

				eqtm::EQCode areaCode;
				areaCode.dt = 0x04 + static_cast<int>(domID << 4);
				areaCode.len = level;
				areaCode.morton = 0;

				EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

				for (EQ_ULLONG morID = 0; morID < n_cell_in_domain; ++morID){
					areaCode.morton = morID;

					std::cout << "creat " << domID << "-" << morID << std::endl;

					mongo::BSONObjBuilder bob;
					createTile(bob, areaCode, dataDirectory);

					conn.insert(tableName, bob.obj());
				}
			}

			std::cout << "creat " << ssTableName.str() << " end" << std::endl;
		}
	}
#endif

#ifdef SRTM_INPUT
	for (unsigned int level = 6; level <= 8; level++){

		if (level % 3 == 0){
			std::stringstream ssTableName;
			ssTableName << dbName << ".Level" << level;

			std::cout << "start creat " << ssTableName.str() << std::endl;

			std::string tableName = ssTableName.str();
			conn.createCollection(tableName);

			for (unsigned int domID = 1; domID < 2; domID++){

				eqtm::EQCode areaCode;
				areaCode.dt = 0x04 + static_cast<int>(domID << 4);
				areaCode.len = level;
				areaCode.morton = 0;

				EQ_ULLONG n_cell_in_domain = (1 << level) * (1 << level);

				for (EQ_ULLONG morID = 0; morID < n_cell_in_domain; ++morID){
					areaCode.morton = morID;

					if (isExist(getFilePath(areaCode, dataDirectory)) == false){
						continue;
					}

					std::cout << "creat " << domID << "-" << morID << std::endl;

					mongo::BSONObjBuilder bob;
					createTile(bob, areaCode, dataDirectory);

					conn.insert(tableName, bob.obj());
				}
			}

			std::cout << "creat " << ssTableName.str() << " end" << std::endl;
		}
	}
#endif

	std::cin.get();
	return 0;
}
