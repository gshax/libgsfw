#!/bin/sh

# make sure we're in the right place
cd `dirname "$0"`

# install dependencies
npm install

echo

# build native tools
cd utils
make
cd ..

# generate env.sh
echo "PATH=\$PATH:`pwd`/bin" > env.sh

# yes, that's really it!
echo
echo "=== INIT COMPLETE ==="
echo "run this to add ./bin to your PATH:"
echo ". `pwd`/env.sh"