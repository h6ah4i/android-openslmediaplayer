#!/bin/bash
#
#    Copyright (C) 2014 Haruki Hasegawa
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

pushd $(dirname $0) > /dev/null

echo "==========================================="

# generate test files
echo "Generating test media files"

pushd ./media > /dev/null
make >> /dev/null
popd >> /dev/null

echo "==========================================="

# server settings
PORT=8080
IPADDR=$(python -c "import socket;print([(s.connect(('8.8.8.8', 80)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1])")

# print server root directory path
echo "ROOT Directory PATH:"
echo "    $(pwd)"
echo

# print server IP address
echo "Server Address:"
echo "    $IPADDR:$PORT"

echo "==========================================="
echo

# lunch HTTP server
if which http-server > /dev/null 2>&1; then
	# http-server
	http-server -p $PORT ./
elif which twistd > /dev/null 2>&1; then
	# Twistd
	twistd -no web --path=. --port=$PORT
elif which python3 > /dev/null 2>&1; then
	# Python 3
	python3 -m http.server $PORT
elif which python2 > /dev/null 2>&1; then
	# Python 2
	python2 -m SimpleHTTPServer $PORT
elif which python > /dev/null 2>&1 ; then
	# Python 2
	python -m SimpleHTTPServer $PORT
else
	echo "Error: No simple HTTP server program found"
fi

popd > /dev/null