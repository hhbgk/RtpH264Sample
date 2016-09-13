package com.rtp.h264.sample;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends AppCompatActivity {
    private EditText mFilenameEdit;
    private Button mSendBtn;
    private RtpClient mRtp;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mRtp = new RtpClient();

        mFilenameEdit = (EditText) findViewById(R.id.edit_id);
        mFilenameEdit.setText("test.h264");

        mSendBtn = (Button) findViewById(R.id.send);
        mSendBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String filePath = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getPath();
                filePath += "/";
                filePath += mFilenameEdit.getText().toString().trim();
                mRtp.send(filePath);
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mRtp != null){
            mRtp.release();
        }
    }
}
