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

set xrange [0:]
set grid x y
set xlabel "Time (sec.)"
set ylabel "Buffer read/write position"
set ytics nomirror
set y2range [0:100]
set y2tics
set y2label "Delay (ms)"
plot \
    'write.log' using 4:(($4-$5)*1000) with linespoints axis x1y2 title "WRITE time delay", \
    'write.log' using 4:3 with linespoints axis x1y1 title "WRITE", \
    'read.log' using 4:3 with linespoints axis x1y1 title "READ"
