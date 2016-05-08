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

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.model.GlobalAppController;
import com.h6ah4i.android.example.openslmediaplayer.app.model.MediaPlayerStateStore;

public class ActionBarTileBuilder {
    public static String makeTitleString(Context context, int page_title_resid) {
        GlobalAppController controller = GlobalAppControllerAccessor.getInstance(context);

        int impl_type_resid;
        switch (controller.getPlayerStateStore().getPlayerImplType()) {
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_STANDARD:
                impl_type_resid = R.string.mediaplayer_impl_standard;
                break;
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_OPENSL:
                impl_type_resid = R.string.mediaplayer_impl_opensl;
                break;
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_HYBRID:
                impl_type_resid = R.string.mediaplayer_impl_hybrid;
                break;
            default:
                return "";
        }

        return context.getString(impl_type_resid) + " - " + context.getString(page_title_resid);
    }
}
