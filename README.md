# BluetoothLEController

Firstly I wanted to warn you and say that you probably should not use this. This [library](https://wikipedia.org/wiki/Library_(computing)) was written specifically for my personal purposes. It provides ease-of-use handling of [Bluetooth Low Energy](https://wikipedia.org/wiki/Bluetooth_Low_Energy)  devices while nearly running everything sequentially. Although you could use it in a separate thread I'd recommend you directly working with the [Bluetooth LE API](https://docs.microsoft.com/windows/uwp/devices-sensors/bluetooth-low-energy-overview) Microsoft provides.

I'm using [regular expression](https://wikipedia.org/wiki/Regular_expression)s to for example be able to connect to different devices from the same company which have the same API but may differ in identification.

## Installation

The only things you need from this repository are [BluetoothLEController.cpp](./BluetoothLEController/BluetoothLEController.cpp) and [BluetoothLEController.hpp](./BluetoothLEController/BluetoothLEController.hpp). Add those files to your project and include the header as usual.

Adjust your projects `Configuration Properties` as follows:

- Add `WindowsApp.lib` to the additional dependencies at `Linker Input`.
- If you use `/std:c++latest` you might need to add `/await` to the additional options at `C/C++ Command Line` as required by `<experimental/coroutine>` and `<experimental/resumable>`.

**Note:** If there are several errors in `Windows.Foundation.0.h` you might need to change `Windows SDK Version 10.0.19041.0` under `General` to `10.0.18362.0` or lower.

#### Make sure that

- Bluetooth is working on your PC and supports Bluetooth Low Energy.
- Your device is turned on, discoverable, connectable and in range.

## Documentation

- `bool ConnectBLEDeviceByID(const std::wregex& deviceIDRegex)`
  Connects to a [BluetoothLEDevice](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.bluetoothledevice) which's ID matches the given regex.
  The function returns a Boolean stating whether the function succeeded. Use `GetLastError()` for information about a failure.
  - `deviceIDRegex`
    A regular expression which should match the [ID of the device](https://docs.microsoft.com/uwp/api/windows.devices.enumeration.deviceinformation.id#Windows_Devices_Enumeration_DeviceInformation_Id).
- `bool ConnectBLEDeviceByName(const std::wregex& deviceNameRegex)`
  Connects to a [BluetoothLEDevice](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.bluetoothledevice) which's name matches the given regex.
  The function returns a Boolean stating whether the function succeeded. Use `GetLastError()` for information about a failure.
  - `deviceNameRegex`
    A regular expression which should match [the name of the device](https://docs.microsoft.com/uwp/api/windows.devices.enumeration.deviceinformation.name#Windows_Devices_Enumeration_DeviceInformation_Name).
    **Keep in mind** that the name can change due to localization.
- `bool SelectService(const std::wregex& serviceGUIDRegex)`
  Selects a [GattDeviceService](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattdeviceservice) on which the notify and write property reside after being connected to a device.
  The function returns a Boolean stating whether the function succeeded. Use `GetLastError()` for information about a failure.
  - `serviceGUIDRegex`
    A regular expression which should match the [GUID of the service](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattdeviceservice.uuid#Windows_Devices_Bluetooth_GenericAttributeProfile_GattDeviceService_Uuid).
    The [GUID](https://docs.microsoft.com/windows/win32/api/guiddef/ns-guiddef-guid) is always of the form `{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}`.
- `bool ConnectBLEDeviceByService(const std::wregex& serviceGUIDRegex)`
  Connects to a [BluetoothLEDevice](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.bluetoothledevice) which has a [GattDeviceService](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattdeviceservice) which's GUID matches the given regex.
  It will select the matched service.
  The function returns a Boolean stating whether the function succeeded. Use `GetLastError()` for information about a failure.
  - `serviceGUIDRegex`
    A regular expression which should match the [GUID of the service](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattdeviceservice.uuid#Windows_Devices_Bluetooth_GenericAttributeProfile_GattDeviceService_Uuid).
    The [GUID](https://docs.microsoft.com/windows/win32/api/guiddef/ns-guiddef-guid) is always of the form `{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}`.
- `bool SelectCharacteristics(const std::function<void(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic sender, winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs eventArgs)>& onValueChanged)`
  Selects [GattChracteristic](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattcharacteristic)s with [GattCharacteristicProperties](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattcharacteristicproperties) `Write` and `Notify`.
  Will set the configuration descriptor of the `Notify` characteristic to `Notify` and adds a callback to  [ValueChanged](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattcharacteristic.valuechanged).
  The function returns a Boolean stating whether the function succeeded. Use `GetLastError()` for information about a failure.
  - `onValueChanged`
    The function that should be executed every time the device notifies.
- `bool GattWrite(const std::wstring& cmd)`
  Performs a characteristic value write to a device.
  The function returns a Boolean stating whether the function succeeded. Use `GetLastError()` for information about a failure.
  - `cmd`
    The data that should be written to the device.
- `bool DisconnectBLEDevice()`
  Closes all connections and disconnects the device.
  The function returns a Boolean stating whether the function succeeded. Use `GetLastError()` for information about a failure.

#### Note
Sites such as [RegExr](https://regexr.com/) and [Regex101](https://regex101.com/) are really helpful when it comes to creating and validating a regular expression.

### Example(s)

##### `ConnectBLEDeviceByID`

```cpp
int main()
{
	BluetoothLEController bLEController;

	std::wregex deviceIDRegex(L"^.*..:..:54:6f:72:61-64:6f:72:61:..:..");
	std::wregex serviceGUIDRegex(L"^\\{..3e4567-e89.-12d3-a456-426652340000\\}");

	if (!bLEController.ConnectBLEDeviceByID(deviceIDRegex))
	{
		return GetLastError();
	}

	if (!bLEController.SelectService(serviceGUIDRegex))
	{
		return GetLastError();
	}

	if (!bLEController.SelectCharacteristics([](
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
			GattCharacteristic sender,
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
			GattValueChangedEventArgs eventArgs)
		{
			std::cout << eventArgs.CharacteristicValue().data() << std::endl;
		}))
	{
		return GetLastError();
	}

	if (!bLEController.GattWrite(L"Ping;"))
    {
        return GetLastError();
    }
	
    if (!bLEController.DisconnectBLEDevice())
    {
        return GetLastError();
    }
}
```

##### `ConnectBLEDeviceByName`

```cpp
int main()
{
	BluetoothLEController bLEController;

	std::wregex deviceNameRegex(L"^.*45MW\.TRG.*");
	std::wregex serviceGUIDRegex(L"^\\{..3e4567-e89.-12d3-a456-426652340000\\}");

	if (!bLEController.ConnectBLEDeviceByName(deviceNameRegex))
	{
		return GetLastError();
	}

	if (!bLEController.SelectService(serviceGUIDRegex))
	{
		return GetLastError();
	}

	if (!bLEController.SelectCharacteristics([](
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
			GattCharacteristic sender,
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
			GattValueChangedEventArgs eventArgs)
		{
			std::cout << eventArgs.CharacteristicValue().data() << std::endl;
		}))
	{
		return GetLastError();
	}

	if (!bLEController.GattWrite(L"Ping;"))
    {
        return GetLastError();
    }
	
    if (!bLEController.DisconnectBLEDevice())
    {
        return GetLastError();
    }
}
```

##### `ConnectBLEDeviceByService`

```cpp
int main()
{
	BluetoothLEController bLEController;

	std::wregex serviceGUIDRegex(L"\\{..3e4567-e89.-12d3-a456-426652340000\\}");

	if (!bLEController.ConnectBLEDeviceByService(serviceGUIDRegex))
	{
		return GetLastError();
	}

	if (!bLEController.SelectCharacteristics([](
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
			GattCharacteristic sender,
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
			GattValueChangedEventArgs eventArgs)
		{
			std::cout << eventArgs.CharacteristicValue().data() << std::endl;
		}))
	{
		return GetLastError();
	}
    
	if (!bLEController.GattWrite(L"Ping;"))
    {
        return GetLastError();
    }
	
    if (!bLEController.DisconnectBLEDevice())
    {
        return GetLastError();
    }
}
```

### Application-Defined [Last-Error Codes](https://docs.microsoft.com/windows/win32/debug/last-error-code)

All public functions (except constructor/destructor _duh_) return a [Boolean](https://wikipedia.org/wiki/Boolean_data_type) indicating whether they were successful or not. Use [GetLastError](https://docs.microsoft.com/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror) to retrieve the thread's last-error code.  Every error code defined by this application has the 29th bit set as stated in [Microsoft docs](https://docs.microsoft.com/windows/win32/debug/last-error-code). Additionally to that the bits 30, 27 and 24 are set.

- Error `1761607680 (0x69000000)`
- [BluetoothLEDevice](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.bluetoothledevice)NotFound `1761607681 (0x69000001)`
- [BluetoothLEDeviceDisconnected](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.bluetoothconnectionstatus) `1761607682 (0x69000002)`
- [GattDeviceService](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattdeviceservice)NotFound `1761607683 (0x69000003)`
- [GattCharacteristics](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattcharacteristic)
  - PropertiesNotFound `1761607684 (0x69000004)`
  - WritePropertyNotFound `1761607685 (0x69000005)`
  - NotifyPropertyNotFound `1761607686 (0x69000006)`
- [GattCommunicationStatus](https://docs.microsoft.com/uwp/api/windows.devices.bluetooth.genericattributeprofile.gattcommunicationstatus)
  - Unreachable `1761607687 (0x69000007)`
  - ProtocolError `1761607688 (0x69000008)`
  - AccessDenied `1761607689 (0x69000009)`

### Problems

Currently the device watcher will continue running until the device was found.

Abortion of the device watcher will just be treated as either `DeviceNotFound` or `ServiceNotFound`.

## Remark

**Ackchyually** `SelectService` should be called `SelectGattService` (or `SelectGattDeviceService`) same for `ConnectBLEDeviceByService` and `SelectCharacteristics` but at one point I couldn't be bothered anymore. I know that my sense of naming can be horrendous but technical names aren't really better. I might correct this one day.

## License
[MIT](https://choosealicense.com/licenses/mit/)