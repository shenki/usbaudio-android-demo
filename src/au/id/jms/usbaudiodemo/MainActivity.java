package au.id.jms.usbaudiodemo;


import java.util.HashMap;
import java.util.Iterator;

import org.libusb.UsbHelper;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import au.id.jms.usbaudio.AudioPlayback;
import au.id.jms.usbaudio.UsbAudio;

public class MainActivity extends FragmentActivity {
	
	private static final String TAG = "UsbAudio";
	
    private static final String ACTION_USB_PERMISSION = "com.minelab.droidspleen.USB_PERMISSION";
    PendingIntent mPermissionIntent = null;
    UsbManager mUsbManager = null;
    UsbDevice mAudioDevice = null;
    
    UsbAudio mUsbAudio = null;

	Thread mUsbThread = null;

	private UsbReciever mUsbPermissionReciever;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        Log.d(TAG, "Hello, World!");
        
        // Grab the USB Device so we can get permission
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
        while(deviceIterator.hasNext()){
            UsbDevice device = deviceIterator.next();
    		UsbInterface intf = device.getInterface(0);
    		if (intf.getInterfaceClass() == UsbConstants.USB_CLASS_AUDIO) {
    			Log.d(TAG, "Audio class device: " + device);
    			mAudioDevice = device;
    		}
        }
        
        // Load native lib
        System.loadLibrary("usb-1.0");
        UsbHelper.useContext(getApplicationContext());
        
    	mUsbAudio = new UsbAudio();
    	
    	AudioPlayback.setup();
    	
    	// Buttons
		final Button startButton = (Button) findViewById(R.id.button1);
		final Button stopButton = (Button) findViewById(R.id.button2);
		
		startButton.setEnabled(true);
		stopButton.setEnabled(false);
		
		startButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Log.d(TAG, "Start pressed");
		    	if (mUsbAudio.setup() == true) {
		    		startButton.setEnabled(false);
		    		stopButton.setEnabled(true);
		    	}
		    	
		        new Thread(new Runnable() {
		            public void run() {
		            	while (true) {
		            		mUsbAudio.loop();
		            	}
		            }
		        }).start();
			}
		});
		
		stopButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Log.d(TAG, "Stop pressed");
		    	mUsbAudio.stop();
		    	mUsbAudio.close();
		    	
	    		startButton.setEnabled(true);
	    		stopButton.setEnabled(false);
			}
		});
        
        // Register for permission
        mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        mUsbPermissionReciever = new UsbReciever();
        registerReceiver(mUsbPermissionReciever, filter);
        
        // Request permission from user
        if (mAudioDevice != null && mPermissionIntent != null) {
        	mUsbManager.requestPermission(mAudioDevice, mPermissionIntent);
        } else {
        	Log.e(TAG, "Device not present? Can't request peremission");
        }
    }
    
    @Override
    protected void onDestroy() {
    	unregisterReceiver(mUsbPermissionReciever);
    	if (mUsbAudio != null) {
    		mUsbAudio.stop();
    		mUsbAudio.close();
    	}
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.activity_main, menu);
        return true;
    }

    private void setDevice(UsbDevice device) {
    	// Set button to enabled when permission is obtained
    	((Button) findViewById(R.id.button1)).setEnabled(device != null);
    }
    
    private class UsbReciever extends BroadcastReceiver {
		
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
			if (ACTION_USB_PERMISSION.equals(action)) {
				if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
					setDevice(device);
				} else {
					Log.d(TAG, "Permission denied for device " + device);
				}
			}
		}
    }
}
