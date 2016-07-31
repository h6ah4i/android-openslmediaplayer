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

import android.content.Context;
import android.media.MediaMetadataRetriever;
import android.net.Uri;

import com.h6ah4i.android.example.openslmediaplayer.app.model.MediaMetadata;

public class MediaMetadataBuilder {

    private MediaMetadataBuilder() {
    }

    public static MediaMetadata create(Context context, Uri uri) {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();

        retriever.setDataSource(context, uri);

        MediaMetadata metadata = new MediaMetadata();

        metadata.setTitile(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE));
        metadata.setArtist(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST));
        metadata.setAlbum(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM));

        return metadata;
    }

}
