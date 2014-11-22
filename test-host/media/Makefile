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

RM=rm -f

.PHONY: clean


all: whitenoise_stereo_3min.wav

whitenoise_stereo_3min.wav:
	sox -r 44100 -c 2 -e signed-integer -b 16 --null $@ synth 3:00 whitenoise vol 0.1

clean:
	$(RM) *.wav

