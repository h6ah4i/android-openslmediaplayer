#/!bin/sh
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

# determine offset time
R1=$(python split_log.py "R" 0 1 < capture.log)
W1=$(python split_log.py "W" 0 1 < capture.log)

TOFFSET=$(echo "$R1\n$W1" | awk 'BEGIN{ offset = 0 } { if (offset == 0 || $4 < offset) offset = $4 } END { print offset }')

python split_log.py "R" "$TOFFSET" -1 < capture.log > read.log
python split_log.py "W" "$TOFFSET" -1 < capture.log > write.log
