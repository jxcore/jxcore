# Universal Windows Platform (uwp)

If you build JXcore with `--engine-chakra` on Windows 10, uwp is accessible from a global object:

```js
var uwp = jxcore.uwp;
```

## Example

```javascript
var uwp = jxcore.uwp;
uwp.projectNamespace("Windows");

// check battery status
var batteryStatus = Windows.System.Power.PowerManager.batteryStatus;
var batteryStatusEnum = Windows.System.Power.BatteryStatus;

if (batteryStatus === batteryStatusEnum.notPresent) {
  console.log('The battery or battery controller is not present.');
} else if (batteryStatus === batteryStatusEnum.discharging) {
  console.log('The battery is discharging.');
} else if (batteryStatus === batteryStatusEnum.idle) {
  console.log('The battery is idle.');
} else if (batteryStatus === batteryStatusEnum.charging) {
  console.log('The battery is charging.');
}

uwp.close();
```

Full Windows api description is available on [msdn](https://msdn.microsoft.com/en-us/library/windows/apps/br211377.aspx).

## API

`jxcore.uwp` has 2 functions.

## projectNamespace(name)

Project a UWP namespace of given name.

* **Note**: This function will keep Node process alive so that your app can
  continue to run and handle UWP async callbacks. You need to call
  [close()](#close) when UWP usage is completed.

<a name="close"></a>

## close()

Close all UWP handles. Call this when all UWP usage is completed.