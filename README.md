# bfproxy

A tiny multi-threaded server for saving protobuf data to mysql; very fast; c++11; 
Do not need stopping server when you update;

Architecture:

       0. master thread dispatch all messages to message queue (very fast);
       1. threads of worker pool finish all messages or report errors;


Thanks:

       0. protobuf https://github.com/protocolbuffers/protobuf
       1. spdlog https://github.com/gabime/spdlog
       2. message_queue https://github.com/LnxPrgr3/message_queue

Install:

       0.install mysql-community-devel-xxxxxx
       1.set BF_PATH at CMakeLists.txt
       2.mv protobuf-3.x.x bfproxy/lib/protobuf   
       3.cd bfproxy/lib/protobuf;./autogen.sh;./configure CXXFLAGS=-fPIC --prefix=`pwd`;make;make install
       4.cd ../../;mkdir build;cd build;cmake ..;make

Usage:

       example 
TODO: 

       1. need a configure file
       2. better performance
       3. less bug




