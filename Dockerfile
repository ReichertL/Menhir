FROM amazonlinux:2023 

RUN yum install make -y
RUN yum install cmake -y
RUN yum install openssl-devel -y
RUN yum install boost-devel -y
RUN yum install gcc-c++ -y
RUN yum install unzip -y
RUN yum install sqlite-devel -y
RUN yum install valgrind-devel -y
RUN yum install python3 -y
RUN python3 -m ensurepip
RUN pip3 install psutil numpy
RUN find / -name "libboost_*" 2>/dev/null

COPY . /menhir
#install Path ORAM library 
WORKDIR /menhir/pathoram
RUN make clean
RUN make shared
WORKDIR /menhir
RUN make copy-libs-dev
RUN make install-libs
#install rpc library for server functionalities
WORKDIR /
RUN curl https://github.com/rpclib/rpclib/archive/refs/tags/v2.3.0.zip -LO
RUN unzip -q v2.3.0.zip 
WORKDIR /rpclib-2.3.0
RUN mkdir build
WORKDIR /rpclib-2.3.0/build
RUN cmake ..
RUN cmake --build .
RUN make install
#build Menhir database
WORKDIR /menhir
RUN make main
RUN python3 runEval_template.py
#install gtest
WORKDIR /
RUN curl https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip -LO
RUN unzip -q v1.14.0.zip 
WORKDIR googletest-1.14.0
RUN cmake -DBUILD_SHARED_LIBS=ON .
RUN make
RUN make install
RUN echo "/usr/local/lib" >> /etc/ld.so.conf.d/gtest.conf
RUN echo "/usr/local/lib64" >> /etc/ld.so.conf.d/boost.conf
RUN echo "/usr/lib" >> /etc/ld.so.conf.d/pathoram.conf
RUN ldconfig
#RUN mv /usr/local/lib/libg* /usr/li#
#Build and run tests
WORKDIR /menhir
RUN make clean
RUN make tests
RUN ldconfig -v
RUN ./bin/test-retrival 

