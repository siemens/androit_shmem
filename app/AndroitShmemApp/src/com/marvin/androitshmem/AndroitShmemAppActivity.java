package com.marvin.androitshmem;

import com.androit.SharedMem;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class AndroitShmemAppActivity extends Activity {
	SharedMem shmem;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        shmem = new SharedMem();
        
        final EditText etBBFloat = (EditText) findViewById(R.id.etBBFloat);
        final EditText etBBInt = (EditText) findViewById(R.id.etBBInt);
        
        Button btGetBBValues = (Button) findViewById(R.id.btGet);
        btGetBBValues.setOnClickListener(new OnClickListener() {			
			@Override
			public void onClick(View v) {
				etBBFloat.setText(String.valueOf(shmem.getBBFloat()));
				etBBInt.setText(String.valueOf(shmem.getBBInt()));
			}
		});
        
        Button btSetBBValues = (Button) findViewById(R.id.btSet);
        btSetBBValues.setOnClickListener(new OnClickListener() {			
			@Override
			public void onClick(View v) {
				shmem.setBBValues(Float.valueOf(etBBFloat.getText().toString()), Integer.valueOf(etBBInt.getText().toString()));		
			}
		});
    }
}