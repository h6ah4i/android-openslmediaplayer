/*
 *    Copyright (C) 2014 Haruki Hasegawa
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


package com.h6ah4i.android.media.test.utils;

import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;

import android.content.res.AssetManager;

import com.h6ah4i.android.media.IReleasable;

public class CommonTestCaseUtils {
    private CommonTestCaseUtils() {
    }

    public static void closeQuietly(Closeable c) {
        if (c == null)
            return;

        try {
            c.close();
        } catch (Exception e) {
        }
    }

    public static void releaseQuietly(IReleasable releasable) {
        if (releasable == null)
            return;

        try {
            releasable.release();
        } catch (Exception e) {
        }
    }

    public static void copyAssetFile(AssetManager am, String assetName, File destBaseDir,
            boolean overwrite)
            throws IOException {
        File destFile = new File(
                destBaseDir
                        .getAbsolutePath()
                        .concat(File.separator)
                        .concat(assetName));

        if (destFile.exists() && !overwrite)
            return;

        if (overwrite) {
            destFile.delete();
        }
        destFile.getParentFile().mkdirs();

        InputStream is = null;
        OutputStream os = null;

        try {
            byte[] buff = new byte[4096];
            is = am.open(assetName, AssetManager.ACCESS_STREAMING);
            os = new FileOutputStream(destFile);

            while (true) {
                int nread = is.read(buff);
                if (nread <= 0)
                    break;
                os.write(buff, 0, nread);
            }
        } finally {
            closeQuietly(is);
            closeQuietly(os);
        }
    }

    public static void copyFile(File sourceFile, File destFile) throws IOException {
        if (!destFile.exists()) {
            destFile.createNewFile();
        }

        FileChannel source = null;
        FileChannel destination = null;

        try {
            source = new FileInputStream(sourceFile).getChannel();
            destination = new FileOutputStream(destFile).getChannel();
            destination.transferFrom(source, 0, source.size());
        } finally {
            closeQuietly(source);
            closeQuietly(destination);
        }
    }

    public static void removeDirectory(File dir) {
        if (dir.isDirectory()) {
            final File[] files = dir.listFiles();
            if (files != null) {
                for (final File file : files) {
                    removeDirectory(file);
                }
            }
            dir.delete();
        } else {
            dir.delete();
        }
    }
}
