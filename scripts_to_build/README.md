## build on Microsoft Windows:

1. Install Visual Studio 2015 from [here](https://go.microsoft.com/fwlink/?LinkId=532606&clcid=0x409)
	1. If performing a custom install be sure to check 
		- Programing Languages > C++ 
		- Windows and Web Development > Windows 8.1 and Windows Phone 8.0/8.1 tools >  Tools and Windows SDKs
	
1. Install Open CV [here](https://sourceforge.net/projects/opencvlibrary/files/opencv-win/)
	1. Take note of the installation location, this will be needed later
	1. Add \path\to\opencv\build\x64\vc14\bin to PATH
	
1. Install Qt [here](https://www.qt.io/download-qt-installer)
	1. You will need to make a Qt account :(, but this can be done via the installer
	1. Select Components
		- Qt 5.13.0 > MSVC 2015 64-bit
		
1. Install Cmake [here](https://cmake.org/download/)

1. Edit `win_make_vc14_x64_to_edit.bat` and modify : 
	1. QT5_DIR="/path/to/Qt/msvcXXX/lib/cmake"
	1. DCMAKE_PREFIX_PATH="/path/to/OpenCV/build;/path/to/Qt/5.13.0/msvc2015_64/lib/cmake/Qt5Xml"
		1. The first path should point to the opencv build folder which contains `OpenCVConfig.cmake`
		2. The second path should point to the Qt5Xml folder which contains `Qt5XmlConfigVersion.cmake`

1. Run `win_make_vc14_x64_to_edit.bat`

1. Run `make.bat`

1. Move `scripts_to_build\build\x64\PixelAnnotationTool_x64_v1.3.2-6-XXXXXX` folder to a more accessible location and rename as desired

1. Run PixelAnnotationTool.exe in above folder

## build and run on linux :

On ubuntu, PixelAnnotationTool need this pacakage (OpenCV and Qt5.9.1): 

1. Install Qt and OpenCV
    ```sh
    sudo apt-get install mesa-common-dev
    sudo apt-get install libopencv-dev python-opencv
    wget http://download.qt.io/official_releases/qt/5.9/5.9.1/qt-opensource-linux-x64-5.9.1.run
    chmod +x qt-opensource-linux-x64-5.9.1.run
    ./qt-opensource-linux-x64-5.9.1.run
    ```
1. Install Cuda 8.0
    ```
    wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1604/x86_64/cuda-repo-ubuntu1604_8.0.61-1_amd64.deb
    sudo dpkg -i cuda-repo-ubuntu1604_8.0.61-1_amd64.deb
    sudo apt-get update
    sudo apt-get install cuda=8.*
    ```
    1. If you are running into NO PUBKEY errors look [here](https://askubuntu.com/questions/20725/gpg-error-the-following-signatures-couldnt-be-verified-because-the-public-key)
1. Edit [build_linux.sh](https://github.com/maraatech/PixelAnnotationTool/blob/master/scripts_to_build/build_linux.sh)
	1. Change -DQT5_DIR to correct QT directory
	1. Change -DCUDA_TOOLKIT_ROOT_DIR to correct Cuda 8.0 directory IF REQUIRED
