#CFLAGS = -g `pkg-config --cflags opencv` -L/usr/local/lib 
CFLAGS = -I/usr/local/include/opencv4 -I./nlopt-2.6.1/src/api -I./lbplibrary/ -std=c++11 -Wall -Wextra -Wno-unused-parameter -g
LFLAGS = -L/usr/local/lib -lopencv_calib3d  -lopencv_core -lopencv_features2d -lopencv_imgcodecs \
-lopencv_flann \
-lopencv_highgui \
-lopencv_imgproc \
-lopencv_ml \
-lopencv_objdetect \
-lopencv_photo \
-lopencv_stitching \
-lopencv_superres \
-lopencv_video 

all: Preprocess

page_dewarp.o: page_dewarp.cpp page_dewarp.hpp
	g++ ${CFLAGS}  -c $< 

C_preprocess.o: C_preprocess.cpp C_preprocess.hpp
	g++ ${CFLAGS} -c $<

main.o: main.cpp
	g++ ${CFLAGS} -c $<

Preprocess: main.o page_dewarp.o C_preprocess.o
	g++ $^ -o $@ ${LFLAGS} -L./nlopt-2.6.1/ -lnlopt -Wl,-rpath=./nlopt-2.6.1/  -L./lbplibrary/ -llbp -Wl,-rpath=./lbplibrary/ 

#page_dewarp: page_dewarp.o page_dewarp.hpp
#	clang++ $< -o page_dewarp ${LFLAGS} -L./nlopt-2.6.1/ -lnlopt -Wl,-rpath=./nlopt-2.6.1/ -llbp -Wl,-rpath=./lbplibrary/

clean:
	rm *.o Preprocess
