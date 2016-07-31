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


package com.h6ah4i.android.media.openslmediaplayer.testing;

import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class ParameterizedTestSuiteBuilder {
    public static interface Filter {
        boolean filter(Method method);
    }

    static public Filter matches(final String... names) {
        return matches(Arrays.asList(names));
    }

    static public Filter notMatches(final String... names) {
        return notMatches(Arrays.asList(names));
    }

    static public Filter matches(final Iterable<String> names) {
        return new Filter() {

            @Override
            public boolean filter(Method method) {
                String methodName = method.getName();

                for (String name : names) {
                    if (name.equals(methodName))
                        return true;
                }

                return false;
            }
        };
    }

    static public Filter notMatches(final Iterable<String> names) {
        return new Filter() {

            @Override
            public boolean filter(Method method) {
                String methodName = method.getName();

                for (String name : names) {
                    if (name.equals(methodName))
                        return false;
                }

                return true;
            }
        };
    }

    public static TestSuite build(Class<? extends TestCase> clazz, Object... params) {
        return buildDetail(clazz, Arrays.asList(params), null, true);
    }

    public static TestSuite build(Class<? extends TestCase> clazz, Iterable<?> params) {
        return buildDetail(clazz, params, null, true);
    }

    public static TestSuite buildDetail(
            Class<? extends TestCase> clazz,
            Iterable<?> params,
            Filter filter,
            boolean includeSuperClassMethods) {

        TestSuite suite = new TestSuite();

        // enumerate testXXX methods
        List<String> testMethods = new ArrayList<String>();
        Method[] methods = (includeSuperClassMethods)
                ? clazz.getMethods()
                : clazz.getDeclaredMethods();

        for (Method method : methods) {
            Class<?>[] paramType = method.getParameterTypes();
            Class<?> returnType = method.getReturnType();
            String name = method.getName();
            int modifiers = method.getModifiers();

            if (Modifier.isPublic(modifiers) &&
                    !Modifier.isStatic(modifiers) &&
                    (paramType.length == 0) &&
                    name.startsWith("test") &&
                    returnType.equals(void.class)) {

                if (filter == null || filter.filter(method)) {
                    testMethods.add(method.getName());
                }
            }
        }

        // get constructor
        Constructor<?> parameterizedConstructor = null;

        try {
            parameterizedConstructor = clazz.getConstructor(ParameterizedTestArgs.class);
        } catch (NoSuchMethodException e) {
        }

        // make testcases
        if (parameterizedConstructor != null) {
            for (String method : testMethods) {
                if (params != null) {
                    for (Object param : params) {
                        addParametarizedTest(suite, parameterizedConstructor, method, param);
                    }
                } else {
                    addParametarizedTest(suite, parameterizedConstructor, method, null);
                }
            }
        } else {
            // fall back
            suite.addTestSuite(clazz);
        }

        return suite;
    }

    private static void addParametarizedTest(
            TestSuite suite,
            Constructor<?> parameterizedConstructor,
            String method, Object param) {
        try {
            ParameterizedTestArgs args = new ParameterizedTestArgs(method, param);
            suite.addTest((TestCase) parameterizedConstructor.newInstance(args));
        } catch (InstantiationException e) {
            throw new IllegalStateException(e);
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        } catch (IllegalArgumentException e) {
            throw new IllegalStateException(e);
        } catch (InvocationTargetException e) {
            throw new IllegalStateException(e);
        }
    }
}
