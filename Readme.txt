一、LibEQCommon、LibEQUtil、LibEQRaster
    之前德朋师兄留下的EQTM的库，未做修改

二、ETOPO1Convertor
    将ETOPO1数据转换成格网瓦片金字塔，每个瓦片为ive格式文件，可生成0~5层的全球地形数据块
    编译时依赖osg、osgEarth、3rdParty、eigen、tclap库，eigen和tclap直接添加到包含目录中即可
    命令行参数示例：--raster F:\data\ETOPO1\etopo1tiff.tif --output C:\Users\LWJie\Desktop\test --minlevel 0 --maxlevel 2 --cell 


三、SRTMConvertor
    将全国SRTM数据转换成格网瓦片金字塔程序，同时ive和dgg格式文件，可生成6~11层的全国地形数据块
    编译时依赖osg、osgEarth、3rdParty、eigen、tclap库
    命令行参数示例：--srtm F:\Data\SRTM30 --output F:\Data\Pyramid --minlevel 6 --maxlevel 8

四、InputToMongoDB
    将生成的金字塔存入MongoDB中，存入后并没有索引，需打开MongoDB客户端手动添加索引
    编译时依赖boost、MongoDB c++ driver库
    命令行参数示例：--dir F:\data\Pyramid --db Pyramid

五、TerrainVisualizatio
    MongoDB+Redis可视化程序
    编译时依赖boost、MongoDB c++ driver、Redis hiredis、osg、osgEarth库
    命令行参数示例：--db Pyramid --url 192.168.1.101

六、TerrainVisualizatio_Files
    ive文件可视化程序
    编译时依赖osg、osgEarth、tclap库
    命令行参数示例：--dir C:\Users\LWJie\Desktop\test\Pyramid --minlevel 0 --maxlevel 2