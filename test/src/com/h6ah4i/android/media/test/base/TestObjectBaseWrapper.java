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


package com.h6ah4i.android.media.test.base;

import junit.framework.Assert;

import com.h6ah4i.android.media.test.utils.ThrowableRunnable;

public abstract class TestObjectBaseWrapper {
    public interface Host {
        void hostTestFail();

        void hostTestFail(String message);

        void hostRunTestOnUiThread(Runnable r);

        void hostRunTestOnUiThread(ThrowableRunnable thr) throws Throwable;
    }

    private Host mHost;
    private long mTestRunnerThreadId;
    private long mUiThreadThreadId;

    protected TestObjectBaseWrapper(Host host) {
        mHost = host;
        mTestRunnerThreadId = getCurrentThreadId();
        invoke(new Runnable() {
            @Override
            public void run() {
                mUiThreadThreadId = getCurrentThreadId();
            }
        });
    }

    protected static long getCurrentThreadId() {
        return Thread.currentThread().getId();
    }

    protected boolean isTestRunnerThread() {
        return (getCurrentThreadId() == mTestRunnerThreadId);
    }

    protected boolean isUiThread() {
        return (getCurrentThreadId() == mUiThreadThreadId);
    }

    protected void invoke(Runnable r) {
        if (isUiThread()) {
            r.run();
        } else if (isTestRunnerThread()) {
            try {
                mHost.hostRunTestOnUiThread(r);
            } catch (RuntimeException ex) {
                throw ex;
            } catch (Throwable th) {
                failUnexpectedThrowable(th);
            }
        } else {
            failCalledFronUnexpectedThreadContext();
        }
    }

    // protected static void fail() {
    // TestCase.fail();
    // }

    protected static void fail(String message) {
        Assert.fail(message);
    }

    protected void failUnexpectedThrowable(Throwable th) {
        fail("Unexpected throwable; " + th);
    }

    protected void failCalledFronUnexpectedThreadContext() {
        fail("Called from unexpected thread context; thread id = " + getCurrentThreadId());
    }

    protected void invoke(ThrowableRunnable thr) throws Throwable {
        if (isUiThread()) {
            thr.run();
        } else if (isTestRunnerThread()) {
            mHost.hostRunTestOnUiThread(thr);
        } else {
            failCalledFronUnexpectedThreadContext();
        }
    }
}
