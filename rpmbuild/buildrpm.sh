#!/bin/bash

# This script creates the rpm build structure, copies the appropriate files to
# the structure, and builds the RPM file for installation of hostinfo.

if [ -z ${HOSTINFO_BUILD_VERSION+x} ]; then
  echo "HOSTINFO_BUILD_VERSION is not set. Run as \"HOSTINFO_BUILD_VERSION=version ./buildrpm.sh\"";
  exit 1
fi


HOSTINFO_BASE_DIR=~/hostinfo
RPMBUILD_DIR=$HOSTINFO_BASE_DIR/tmp

echo "Starting hostinfo buildrpm.sh!"
echo "HOSTINFO_BUILD_VERSION=$HOSTINFO_BUILD_VERSION"
echo "HOSTINFO_BASE_DIR=$HOSTINFO_BASE_DIR"
echo "RPMBUILD_DIR=$RPMBUILD_DIR"

# Build the binary
cd $HOSTINFO_BASE_DIR && make clean && make

# Create structure
echo "Creating rpmbuild directory structure..."
SOURCE_DIR="${RPMBUILD_DIR}/SOURCES/hostinfo-${HOSTINFO_BUILD_VERSION}"
mkdir -vp $SOURCE_DIR
mkdir -vp $RPMBUILD_DIR/BUILD
mkdir -vp $RPMBUILD_DIR/BUILDROOT 
mkdir -vp $RPMBUILD_DIR/RPMS/i386 
mkdir -vp $RPMBUILD_DIR/RPMS/i686
mkdir -vp $RPMBUILD_DIR/RPMS/noarch
mkdir -vp $RPMBUILD_DIR/RPMS/x86_64
mkdir -vp $RPMBUILD_DIR/SPEC
mkdir -vp $RPMBUILD_DIR/SRPMS  
mkdir -vp $RPMBUILD_DIR/tmp
echo "[Done creating rpmbuild directory ]"

# Copy over files
echo "Copying release files to rpmbuild directory structure..."
cp -fv $HOSTINFO_BASE_DIR/hostinfo $SOURCE_DIR/

cp -fv $HOSTINFO_BASE_DIR/rpmbuild/hostinfo.spec $RPMBUILD_DIR/SPEC/
echo "[Done copying release files to rpmbuild directory structure]"

# Create tar.gz archive
echo "Creating tar.gz archive..."
cd $RPMBUILD_DIR/SOURCES
tar -czvf hostinfo-${HOSTINFO_BUILD_VERSION}.tar.gz hostinfo-${HOSTINFO_BUILD_VERSION}/
echo "[Done creating tar.gz archive]"

# Build the rpm
echo "Creating the final rpm..."
cd $RPMBUILD_DIR/SPEC
rpmbuild -bb --define "_topdir $RPMBUILD_DIR" --define "_tmppath $RPMBUILD_DIR/tmp" hostinfo.spec
RPMARCH=$(uname -p)
echo "RPM: $RPMBUILD_DIR/RPMS/$RPMARCH/hostinfo-$HOSTINFO_BUILD_VERSION-1.$RPMARCH.rpm"
echo "[Done creating the final rpm]"
