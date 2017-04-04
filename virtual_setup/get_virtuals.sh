#!/bin/bash
# inputenc: utf8

# Download and unpack:
if [[ ! -f ubuntu64-1.img ]] || [[ ! -f ubuntu64-1-mptcp.img ]]; then
    echo "Downloading images files.."

    # Alternative download mirror
    #wget http://kom.aau.dk/group/13gr810/virtuals.tar.bz2
    wget https://www.dropbox.com/s/qucjmillkuo1t25/virtuals.tar.bz2?dl=0

    echo "Extracting files.."
    tar -xvf virtuals.tar.bz2
fi

# Verify downloaded image files
echo "Verifying image files.."
md5sum -c MD5SUMS

#Delete compressed file:
echo "Cleaning up."
rm virtuals.tar.bz2
