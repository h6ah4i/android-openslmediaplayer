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

import sys
import re

rw_tag = sys.argv[1]
offset_time = float(sys.argv[2])
max_count = int(sys.argv[3])


if rw_tag == "R":
	p = re.compile('^.\/H?Q?VisualizerCapturedAudioDataBuffer\([ 0-9]+\): \[(R!?)\] count = ([0-9]+), pos = ([0-9]+), time = \(([0-9]+), ([0-9]+)\).*?$')
elif rw_tag == "W":
	p = re.compile('^.\/H?Q?VisualizerCapturedAudioDataBuffer\([ 0-9]+\): \[(W!?)\] count = ([0-9]+), pos = ([0-9]+), time = \(([0-9]+), ([0-9]+)\), datatime = \(([0-9]+), ([0-9]+)\).*?$')


processed = 0

for line in iter(sys.stdin.readline, ""):
	m = p.match(line)

	if m and m.group(1) == "R":
		tag = m.group(1)
		count = int(m.group(2))
		pos = int(m.group(3))
		time = int(m.group(4)) + (int(m.group(5)) * 0.000000001)

		print \
			tag + "\t" + \
			str(count) + "\t" + \
			str(pos) + "\t" + \
			str(time - offset_time)

		processed += 1

	elif m and m.group(1) == "W":
		tag = m.group(1)
		count = int(m.group(2))
		pos = int(m.group(3))
		time = int(m.group(4)) + (int(m.group(5)) * 0.000000001)
		datatime = int(m.group(6)) + (int(m.group(7)) * 0.000000001)

		print \
			tag + "\t" + \
			str(count) + "\t" + \
			str(pos) + "\t" + \
			str(time - offset_time) + "\t" + \
			str(datatime - offset_time)

		processed += 1

	if max_count > 0 and processed >= max_count:
		break
