package com.example.avfunplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ImageButton;
import android.widget.SeekBar;

import com.avfuncore.AVFPlayer;
import com.avfuncore.view.AVFSurfaceView;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private static final String TAG = "avfunplayer";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initView();
    }

    private void initView() {
        AVFSurfaceView surPreview = findViewById(R.id.sur_preview);
        ImageButton btnPlay = findViewById(R.id.btn_play);
        SeekBar barSeek = findViewById(R.id.bar_seek);
        Log.i("hecc","initView");

        btnPlay.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {

        String absolutePath = getCacheDir().getAbsolutePath();

        // /data/user/0/com.example.avfunplayer/cache
        Log.i(TAG, "absolutePath: "+absolutePath);

        String filename = absolutePath + "/out2.mp4";

        AVFPlayer player = AVFPlayer.Create(filename);
        player.play();
    }
}