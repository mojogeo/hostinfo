# hostinfo

Provides REST interface for host information (using cpprestsdk on RHEL7/CentOS7). This project was created as kind of a baseiline example for setting up other REST APIs I'm considering using Microsoft's Casablanca (cpprestsdk) for. The project has been built/tested on RHEL7 and CentOS7 (the target environment for other projects I'm using cpprestsdk for) 

POC: Chris Juszak

--------------------------------------------------------------------------------
# Development environment notes

## Operating system - RHEL7, 64bit

Built/tested on Red Hat Enterprise Linux (RHEL) 7.5 with updates, x86_64

Suggested configuration
````bash
sudo yum -y update
sudo yum -y install \
  https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
sudo yum -y install autoconf cpio expat-devel gettext-devel make openssl-devel \
  perl-ExtUtils-MakeMaker zlib-devel cmake3 uwsgi-devel zlib-devel \
  libicu-devel devtoolset-7 rpm-build
````

## Compiler/linker - GCC 7.3

Built tested with GCC 7.3.

Suggested configuration
````bash
sudo yum -y install devtoolset-7
echo "source scl_source enable devtoolset-7 &> /dev/null" >> ~/.bashrc
source ~/.bashrc
```` 

## Build dependencies

### Argh

includes/argh.h (v1.2.1) - copied from https://github.com/adishavit/argh

argh.h is used to help with parsing command line options in hostinfo.

argh.h is already copied in the hostinfo git repo, but can be updated like this:
````bash
cd ~/hostinfo/include
wget https://raw.githubusercontent.com/adishavit/argh/v1.2.1/argh.h
````

### Boost

This is a requirement for cpprestsdk (Microsoft Casablanca).

Suggested configuration
````bash
mkdir -p ~/src && cd ~/src
wget https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
tar xf boost_1_68_0.tar.gz
cd boost_1_68_0
./bootstrap.sh --libdir=/usr/local/lib64 \
  --with-libraries=system,thread,test,chrono,regex,date_time,filesystem,locale,random,atomic,log,program_options
./b2 -j 8 cxxflags="-Wno-deprecated-declarations -Wno-unused-function"
sudo ./b2 install
````

### Websocketpp

This is a requirement for cpprestsdk (Microsoft Casablanca).

Suggested configuration
````bash
mkdir -p ~/src && cd ~/src
git clone https://github.com/zaphoyd/websocketpp.git
cd websocketpp
mkdir build && cd build && cmake3 ..
make
sudo make install
````

### Microsoft Casablanca, cpprestsdk

This is the library used to run a HTTP/HTTPS server for the REST API

Suggested configuration
````bash
mkdir -p ~/src && cd ~/src
git clone https://github.com/Microsoft/cpprestsdk.git
cd cpprestsdk
git reset --hard v2.10.7
git submodule update --init
mkdir Release/build.release
cd Release/build.release
CC=gcc CXX=g++ cmake3 .. -DCMAKE_BUILD_TYPE=Release -DWERROR=OFF \
  -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF -DBUILD_SHARED_LIBS=OFF
make -j 8
sudo make install
sudo ldconfig /usr/local/lib /usr/local/lib64/
````

--------------------------------------------------------------------------------
# Building

````bash
cd ~/hostinfo
make
````

--------------------------------------------------------------------------------
# Running/testing

## Run options

To see a list of run options:
````bash
cd ~/hostinfo
./hostinfo -h
````
The output will be similar to the following:

````
Usage: hostinfo ...
Options:
  -h [HOST], --host [host]       IP host to listen on
                                 (default: localhost).
  -p [PORT], --port [port]       IP port to listen on
                                 (1025 to 65535, default: 55111).
                                 (Binding to 80 or 443 if permitted).
  -e, --encrypt                  Use encryption with HTTPS.
  -k [FILE], --key [FILE]        Private key file location for HTTPS
                                 (default: key.pem).
  -c [FILE], --cert [FILE]       Certificate chain file location for HTTPS
                                 (default: cert.pem).
  -v, --verbose                  Provide additional information.
  -h, --help                     Print this help message and quit.
````

### Running in HTTP (non encrypted) mode

````bash
cd ~/hostinfo
./hostinfo --host localhost --port 8080 --verbose
````
This will start a web server that can be accessed at http://localhost:8080

Check with a web browser, or use curl:
````bash
curl http://localhost:8080/archives | python -mjson.tool
````
NOTE: hostinfo can also listen on port 80 (HTTP) but needs system privileges (e.g. sudo ./hostinfo --port 80 ...)

### Running in HTTPS (encrypted) mode

To create a self-signed private key and certificate for testing:
````bash
cd ~/hostinfo
openssl req -newkey rsa:2048 -nodes \
  -keyout key.pem -x509 -days 365 -out cert.pem \
  -subj "/C=US/ST=Colorado/L=FortCollins/O=Foo Name/OU=Org/CN=localhost"
````
WARNING: Common name (CN) must match the "host" option in hostinfo.

To start hostinfo in encrypted/HTTPS mode using a private key and certificate:
````bash
cd ~/hostinfo
./hostinfo --host localhost --port 8081 --verbose --encrypt --key key.pem --cert cert.pem
````
This will start a web server that can be accessed at https://localhost:8081

Check with a web browser, or use curl:
````bash
curl https://localhost:8081/archives | python -mjson.tool
````
NOTE: hostinfo can also listen on port 443 (HTTPS) but needs system privileges (e.g. sudo ./hostinfo --port 443 ...)

# RPM/installer

## Building

````bash
cd ~/hostinfo/rpmbuild
HOSTINFO_BUILD_VERSION=0.0.1 ./buildrpm.sh
````

## Installing

````bash
sudo yum -y install ~/hostinfo/tmp/RPMS/x86_64/hostinfo-0.0.1-1.el7.x86_64.rpm
````
(hostinfo command should now be available)

## Uninstalling

````bash
sudo yum -y remove hostinfo
````

# API

The main webpage of the hostinfo service will list some options that can be viewed in a web browser and/or curl (or anything else that understands HTTP/HTTPS and JSON). Here's some hostinfo URIs that provide JSON results...

/service/info: 	Information/test page

/hardware: Get list of hardware reported by lshw

/processes?LimitResults=10: Get list of processes reported by top (optionally limited by "LimitResults" parameter)

packages?LimitResults=10: Get list of software packages reported by rpm (optionally limited by "LimitResults" parameter)

/sysinfo: Get list of system information reported by uname