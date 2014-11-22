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


package com.h6ah4i.android.example.openslmediaplayer.app.utils;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.MainActivity;
import com.h6ah4i.android.example.openslmediaplayer.app.model.MediaMetadata;

public class NotificationBuilder {
    private NotificationBuilder() {
    }

    public static Notification createServiceNotification(Context context, MediaMetadata metadata) {
        Intent notificationIntent = new Intent(context, MainActivity.class);

        notificationIntent.setFlags(
                Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        notificationIntent.setAction(Intent.ACTION_MAIN);

        final PendingIntent contentIntent = PendingIntent.getActivity(context,
                0, notificationIntent, 0);

        final Notification notification = (new NotificationCompat.Builder(
                context)).setSmallIcon(R.drawable.ic_launcher)
                .setContentIntent(contentIntent)
                .setTicker(null)
                .setContentTitle(metadata.getTitile())
                .setContentText(metadata.getArtist())
                .setOngoing(true)
                .setWhen(0)
                .build();

        return notification;
    }
}
