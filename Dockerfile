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

COPY . /menhir
#install Path ORAM library 
WORKDIR /menhir/pathoram
RUN make shared
WORKDIR /menhir
RUN make copy-libs-dev
RUN make install-libs
#install rpc library for server functionalities
WORKDIR /
RUN curl https://github.com/rpclib/rpclib/archive/refs/tags/v2.3.0.zip -LO
RUN unzip v2.3.0.zip
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
