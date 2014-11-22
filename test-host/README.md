OpenSLMediaPlayerTest - Host machine helper program
===

What's this?
----

Helper program for OpenSLMediaPlayer library's test suite.

Environments
---

- Mac OSX 10.9
- Ubuntu 11.04
- Windows 8.1 (+ Cygwin)


Dependencies
---

- SoX
- Python 2.7.x


Usage
---

1. Lunch server program

    ```
    $ ./run-server.sh
    ===========================================
    Generating test media files
    ===========================================
    ROOT Directory PATH:
        /foo/bar/git/android-openslmediaplayer/test-host
    
    Server Address:
        192.168.11.6:8080                        <-- !!! TEST_SERVER_ROOT !!!
    ===========================================
    
    Serving HTTP on 0.0.0.0 port 8080 ...

    ```

2. Tweak TEST_SERVER_ROOT variable on target test code

- <repository>/test/src/com/h6ah4i/android/media/test/base/GlobalTestOptions.java

    ```java
    // XXX Need to modify the TEST_SERVER_ROOT value on your testing environment
    // 
    // Host HTTP server program:
    //     <repository>/test-host/run-server.sh
    public static final String TEST_SERVER_ROOT = "192.168.11.6:8080";
    ```

3. Turn off 3G/LTE on your device and enable WiFi
4. Run testcases from Eclipse
