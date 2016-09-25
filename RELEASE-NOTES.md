## 0.7.1

- Fixed 11.025 kHz, 12 kHz, 22.05 kHz and 24 kHz audio playback issue (issue #20)
- Update FFTW, libsamplerate, soxr and PFFFT libraries


## 0.7.0

- Added `HybridMediaPlayerFactory` which creates `AudioTrack` audio sink player. It is more tolerant against glitches and strongly recommended than normal `OpenSLMediaPlayer`.
- Fully migrated to Gradle build system. Eclipse is no longer supported.
- Bug fixes


## 0.6.0

- Changed the Standard* prefixed classes not to inherit Android framework classes
- Upgrade Android Gradle build tool version
- Bug fixes
- Code refactoring

## 0.5.0

- Initial release
