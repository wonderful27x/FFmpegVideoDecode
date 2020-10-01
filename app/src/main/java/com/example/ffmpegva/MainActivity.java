package com.example.ffmpegva;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private SurfaceView surfaceView;
    private Button button;
    private WonderfulPlayer wonderfulPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surfaceView);
        button = findViewById(R.id.play);
        wonderfulPlayer = new WonderfulPlayer();
        wonderfulPlayer.setSurfaceHolder(surfaceView);

        File file = new File(Environment.getExternalStorageDirectory() + File.separator,"love.mp4");           //mp4源文件
        File yuvFile = new File(Environment.getExternalStorageDirectory() + File.separator,"loveYuv.txt");     //一帧yuv数据
        File yuvVideoFile = new File(Environment.getExternalStorageDirectory() + File.separator,"loveYuv.yuv");//yuv原始数据
        File yuvPgmFile = new File(Environment.getExternalStorageDirectory() + File.separator,"loveYuv.pgm");  //pgm数据
        final String videoPath = file.getAbsolutePath();
        final String yuvPath = yuvFile.getAbsolutePath();
        final String yuvVideoPath = yuvVideoFile.getAbsolutePath();
        final String yuvPgmPath = yuvPgmFile.getAbsolutePath();
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                wonderfulPlayer.startPlay(videoPath,yuvPath,yuvVideoPath,yuvPgmPath);
                Toast.makeText(MainActivity.this,videoPath,Toast.LENGTH_SHORT).show();
            }
        });

        permission();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,String[] permissions, int[] grantResults){
        switch (requestCode){
            case 1:
                if (grantResults.length>0&&grantResults[0]== PackageManager.PERMISSION_GRANTED){
                }else{
                    Toast.makeText(this,"拒绝权限将无法使用此功能",Toast.LENGTH_SHORT).show();
                }
                break;
            default:
        }
    }

    private void permission(){
        if(ContextCompat.checkSelfPermission(this, Manifest.permission.
                WRITE_EXTERNAL_STORAGE)!= PackageManager.PERMISSION_GRANTED){
            ActivityCompat.requestPermissions(this,new String[]
                    {Manifest.permission.WRITE_EXTERNAL_STORAGE},1);
        }
    }
}
