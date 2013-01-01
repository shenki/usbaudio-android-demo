package au.id.jms.usbaudiodemo;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.util.Log;

public class UsbComms {
	
	private static final String TAG = "UsbComms";
	
    private static final String ACTION_USB_PERMISSION = "com.minelab.droidspleen.USB_PERMISSION";

	private UsbManager mUsbManager;
	private UsbDevice mDevice;
	private UsbInterface mIntf;
	private UsbDeviceConnection mConnection;
	private UsbEndpoint mLineInEp;
	private UsbEndpoint mEpIn;
	
	private PendingIntent mUsbIntent = null;
	private boolean pendingUsbPermission = false;
	
	private Context context;
	
	public UsbComms(Context context) {
		this.context = context;
		this.mUsbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
	}
	
	public void init() {
		if (mUsbIntent == null) {
			mUsbIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
			// Register for broadcast receiver permission.
			IntentFilter filter = new IntentFilter();
			filter.addAction(ACTION_USB_PERMISSION);
			filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
			filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
			context.registerReceiver(new UsbReciever(), filter);
		}
	}
	
	private boolean isAudioDevice(UsbDevice device) {
		UsbInterface intf = device.getInterface(0);
		if (intf.getInterfaceClass() == UsbConstants.USB_CLASS_AUDIO) {
			Log.d(TAG, "Audio class");
			return true;
		} else {
			return false;
		}
		
	}
	
	private void setDevice(UsbDevice device) {
		if (device == null) {
			Log.d(TAG, "Removing device");
			mLineInEp = null;
			return;
		}
		Log.d(TAG, "Interface count: " + device.getInterfaceCount());
		
		UsbInterface intf = device.getInterface(0);
		if (intf.getInterfaceClass() != UsbConstants.USB_CLASS_AUDIO) {
			Log.d(TAG, "Not audio class");
			return;
		}
		mDevice = device;
		
			UsbDeviceConnection connection = mUsbManager.openDevice(device);
			if (connection != null && connection.claimInterface(intf, true)) {
				Log.d(TAG, "Opened device");
				mIntf = intf;
	
				mLineInEp = null;
				for (int i = 0; i < intf.getEndpointCount(); i++ ) {
					UsbEndpoint ep = intf.getEndpoint(i);
					if (ep.getDirection() == UsbConstants.USB_DIR_IN &&
							(ep.getType() & UsbConstants.USB_ENDPOINT_XFERTYPE_MASK) == UsbConstants.USB_ENDPOINT_XFER_ISOC) {
						Log.d(TAG, "Found input endpoint");
						mLineInEp = ep;
					}
				}
			}
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
			} else if (UsbManager.ACTION_USB_ACCESSORY_ATTACHED.equals(action)) {
				if (isAudioDevice(device)) {
					setDevice(device);
				}
			} else if (UsbManager.ACTION_USB_ACCESSORY_DETACHED.equals(action)) {
				if (isAudioDevice(device)) {
					setDevice(null);
				}
			}
			
		}
	};

}
