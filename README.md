OpenSLMediaPlayer
===============

Re-implementation of Android's [MediaPlayer](http://developer.android.com/reference/android/media/MediaPlayer.html) and [audio effect classes](http://developer.android.com/reference/android/media/audiofx/package-summary.html) based on OpenSL ES APIs.

[![Android Arsenal](https://img.shields.io/badge/Android%20Arsenal-OpenSLMediaPlayer-brightgreen.svg?style=flat)](https://android-arsenal.com/details/1/1434)

Motivation
---

I decided to develop this library to solve these frustrations.

- Android OS does not permit to create extension of [`AudioEffect`](http://developer.android.com/reference/android/media/audiofx/AudioEffect.html) in application layer
- OpenSL ES is a primitive API sets, and it is difficult to use than [`MediaPlayer`](http://developer.android.com/reference/android/media/MediaPlayer.html) class in Java
- [`Visualizer`](http://developer.android.com/reference/android/media/audiofx/Visualizer.html) provides very poor quality captured data (8 bit, monaural, 1024 samples/capture, 20 sampling/second)
- [`Visualizer`](http://developer.android.com/reference/android/media/audiofx/Visualizer.html) does not work properly on some devices
    - [android Issue 64423 "Visualizer with audio session 0 does not always receive all of output mix"](https://code.google.com/p/android/issues/detail?id=64423)
    - [felixpalmer/android-visualizer "Visualization does not work on the new Galaxy devices"](https://github.com/felixpalmer/android-visualizer/issues/5)
    - [Music Visualizer "Limitations on Galaxy S3, S4 and Note 3 (Snapdragon Quad core model)"](https://plus.google.com/110898804228810455198/posts/jXGKDLt9iTz)
    - [Music Visualizer "Limitations on Nexus 4, 5, and 7 (2013)"](https://plus.google.com/110898804228810455198/posts/6chmkb9ix1s)
- Android's [MediaPlayer](http://developer.android.com/reference/android/media/MediaPlayer.html) and [audio effect classes](http://developer.android.com/reference/android/media/audiofx/package-summary.html) are buggy...

Features
---

### Advantages

- Provides both C++ and Java API sets
- Smooth fade in/out when starts/pauses playback
- High quality resampler
- 10 bands graphic equalizer & pre. amplifier
- High quality Visualizer class (floating point, stereo, 32k samples/capture, 60 sampling/second)

### Disadvantages

- **Does not support video playback, audio only** ( Please use [`MediaPlayer`](http://developer.android.com/reference/android/media/MediaPlayer.html) or [`ExoPlayer`](http://developer.android.com/guide/topics/media/exoplayer.html) for video playback)
- Consumes more CPU resources than standard MediaPlayer and other AudioTrack/OpenSL based audio player products (ex. PowerAmp)

### Misc.

- Highly compatibility with standard [`MediaPlayer`](http://developer.android.com/reference/android/media/MediaPlayer.html) and [`audio effect classes`](http://developer.android.com/reference/android/media/audiofx/package-summary.html)
- Implements a lot of workarounds, more better behavior and well-tested ([`Standard*` prefixed API classes](../../wiki/OpenSL-prefixed-API-classes))
- Provides Hybrid media player factory which is a player using OpenSL ES for decoding audio and using `AudioTrack` for playback. This is the most recommended `MediaPlayer` because it provides more tolerance for audio glitches than `OpenSLMediaPlayer` but it can use all features of `OpenSLMediaPlayer`!
- Provides some compatibility class or methods for older devices
- NEON/SSE optimized


Target platforms
---

- Android API level 14+   (since this library depends decoding feature introduced from API level 14)


Latest version
---

- Version 0.7.3  (April 8, 2017)


Demo application (pre-built example apps)
---

See [GitHub Pages site of this project](http://h6ah4i.github.io/android-openslmediaplayer/).


Getting started
---

### Use pre-built library (for Android Studio only)

1. Add these lines to `build.gradle` of your project

```groovy
dependencies {
    compile 'com.h6ah4i.android:openslmediaplayer:0.7.3'
}
```

2. That's all ;)

### Build from source

If you want to build this library from source, please refer to [this article](../../wiki/Build-From-Source).

Documentation
---

Refer to [Wiki](../../wiki) and [JavaDoc](https://h6ah4i.github.io/android-openslmediaplayer/javadoc/0.7.2/).


ToDos
---

- More optimize
- Improve JavaDoc comments


License
---

This library is licensed under the [Apache Software License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0), and contains some source code files delivered from product of Android Open Source Project (AOSP). See [`AOSP.md`](AOSP.md) for details.

*Note that this library uses some third party libraries, so you also have to take care of their licenses.*

---

### License of OpenSLMediaPlayer (this library)

See [`LICENSE`](LICENSE) for full of the license text.

    Copyright (C) 2014-2015 Haruki Hasegawa

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


---

### License of dependent libraries

#### cxxdasp, cxxporthelper, OpenSL ES CXX

These libraries are licensed under the [Apache Software License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

- [cxxdasp](https://github.com/h6ah4i/cxxdasp)
- [cxxporthelper](https://github.com/h6ah4i/cxxporthelper)
- [lockfreedatastructure](https://github.com/h6ah4i/lockfreedatastructure)
- [OpenSL ES CXX](https://github.com/h6ah4i/openslescxx)


#### PFFFT

This library is licensed under the [FFTPACK5 Software License](https://www2.cisl.ucar.edu/resources/legacy/fft5/license).

- [PFFFT](https://bitbucket.org/jpommier/pffft)
- [PFFFT (h6a h4i forked version)](https://bitbucket.org/h6a_h4i/pffft)


#### Ne10

This library is licensed under the [BSD 3-Clause license](http://opensource.org/licenses/BSD-3-Clause).

- [Ne10](http://projectne10.github.io/Ne10/)
