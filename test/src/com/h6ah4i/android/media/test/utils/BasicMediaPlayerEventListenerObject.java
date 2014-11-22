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

public abstract class BasicMediaPlayerEventListenerObject {

    private final Object mSyncObj;
    private volatile boolean mTriggered;

    public BasicMediaPlayerEventListenerObject() {
        mSyncObj = new Object();
    }

    public BasicMediaPlayerEventListenerObject(Object syncObj) {
        if (syncObj == null)
            throw new IllegalArgumentException();
        mSyncObj = syncObj;
    }

    @Override
    public String toString() {
        return getClass().getSimpleName() +
                " [" +
                "triggered = " + mTriggered +
                "]";
    }

    protected Object syncObj() {
        return mSyncObj;
    }

    protected void trigger() {
        synchronized (syncObj()) {
            mTriggered = true;
            syncObj().notifyAll();
        }
    }

    public boolean occurred() {
        synchronized (syncObj()) {
            return mTriggered;
        }
    }

    public boolean await(int timeout) {
        final long deadline = System.currentTimeMillis() + timeout;
        synchronized (syncObj()) {
            while (true) {
                if (mTriggered)
                    return true;

                if (timeout == 0)
                    return false;

                final long cur = System.currentTimeMillis();

                if (cur >= deadline)
                    return false;

                try {
                    syncObj().wait(deadline - cur);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    public static boolean awaitAny(int timeout, BasicMediaPlayerEventListenerObject... objs) {

        if (objs == null || objs.length == 0)
            return false;

        Object syncObj = null;

        syncObj = vefiryAndObtainShardSyncObj(objs);

        final long deadline = System.currentTimeMillis() + timeout;

        synchronized (syncObj) {
            while (true) {
                for (BasicMediaPlayerEventListenerObject obj : objs) {
                    if (obj == null)
                        continue;

                    if (obj.occurred())
                        return true;
                }

                final long cur = System.currentTimeMillis();

                if (cur >= deadline)
                    return false;

                try {
                    syncObj.wait(deadline - cur);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    public static boolean awaitAll(int timeout, BasicMediaPlayerEventListenerObject... objs) {

        if (objs == null || objs.length == 0)
            return false;

        Object syncObj = null;

        syncObj = vefiryAndObtainShardSyncObj(objs);

        final long deadline = System.currentTimeMillis();

        synchronized (syncObj) {
            while (true) {
                boolean all = false;

                for (BasicMediaPlayerEventListenerObject obj : objs) {
                    if (obj == null)
                        continue;

                    if (!obj.occurred()) {
                        all = false;
                        break;
                    }
                }

                if (!all)
                    return false;

                final long cur = System.currentTimeMillis();

                if (cur >= deadline)
                    return false;

                try {
                    syncObj.wait(deadline - cur);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    private static Object vefiryAndObtainShardSyncObj(BasicMediaPlayerEventListenerObject... objs) {
        Object syncObj = null;

        for (BasicMediaPlayerEventListenerObject obj : objs) {
            if (obj == null)
                continue;

            if (syncObj == null) {
                syncObj = obj.syncObj();
            } else {
                if (syncObj != obj.syncObj()) {
                    throw new IllegalStateException("Event objects must share the same sync object");
                }
            }
        }

        return syncObj;
    }

    protected boolean triggered() {
        return mTriggered;
    }
}
