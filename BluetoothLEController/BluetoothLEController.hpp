// Copyright 2021, Acurisu
// Author: Acurisu
// E-Mail: acurisu@gmail.com
// Licensed under the MIT License.

#pragma once

#include <functional>
#include <regex>

#include <Unknwn.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Storage.Streams.h>

namespace BluetoothLEControllerErrorCodes
{
	static constexpr DWORD Error = 0x69000000;
	static constexpr DWORD BluetoothLEDeviceNotFound = Error + 0x1;
	static constexpr DWORD BluetoothLEDeviceDisconnected = Error + 0x2;
	static constexpr DWORD GattDeviceServiceNotFound = Error + 0x3;

	namespace GattCharacteristics
	{
		static constexpr DWORD PropertiesNotFound = GattDeviceServiceNotFound + 0x1;
		static constexpr DWORD WritePropertyNotFound = GattDeviceServiceNotFound + 0x2;
		static constexpr DWORD NotifyPropertyNotFound = GattDeviceServiceNotFound + 0x3;
	}

	namespace GattCommunicationStatus
	{
		static constexpr DWORD Unreachable = GattCharacteristics::NotifyPropertyNotFound + 0x1;
		static constexpr DWORD ProtocolError = GattCharacteristics::NotifyPropertyNotFound + 0x2;
		static constexpr DWORD AccessDenied = GattCharacteristics::NotifyPropertyNotFound + 0x3;
	}
}

class BluetoothLEController
{
private:
	inline static const winrt::hstring isConnected = L"System.Devices.Aep.IsConnected";
	inline static const winrt::hstring isConnectable = L"System.Devices.Aep.Bluetooth.Le.IsConnectable";
	enum class Selector { byDeviceID, byDeviceName, byServiceGUID };
	winrt::Windows::Devices::Bluetooth::BluetoothLEDevice bluetoothLEDevice = nullptr;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService gattDeviceService = nullptr;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic gattCharacteristicWrite = nullptr;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic gattCharacteristicNotify = nullptr;

	static bool IsWatcherStarted(const winrt::Windows::Devices::Enumeration::DeviceWatcher& watcher);
	static bool LookupBooleanProperty(const winrt::Windows::Devices::Enumeration::DeviceInformation& deviceInfo, winrt::hstring const& property);
	static bool HasFlag(uint32_t value, uint32_t flags);

	bool BLEDeviceExistsAndConnected();
	bool SelectBLEDeviceService(const winrt::Windows::Devices::Bluetooth::BluetoothLEDevice& device, const std::wregex& serviceGUIDRegex);
	bool SelectBLEDevice(const winrt::Windows::Devices::Enumeration::DeviceInformation& deviceInfo, const std::wregex& regex, Selector selector);
	bool ConnectBLEDevice(const std::wregex& regex, Selector selector);

public:
	/// <summary>
	/// Connects to a BluetoothLEDevice which's ID matches the given regex.
	/// </summary>
	/// <param name="deviceIDRegex">
	/// A regular expression which should match the ID of the device.
	/// </param>
	/// <returns>
	/// A boolean stating whether the function succeeded. Use GetLastError() for information about a failure.
	/// </returns>
	bool ConnectBLEDeviceByID(const std::wregex& deviceIDRegex);
	/// <summary>
	/// Connects to a BluetoothLEDevice which's name matches the given regex.
	/// </summary>
	/// <param name="deviceNameRegex">
	/// A regular expression which should match the name of the device.
	/// </param>
	/// <returns>
	/// A boolean stating whether the function succeeded. Use GetLastError() for information about a failure.
	/// </returns>
	/// <remarks>
	/// The device name can change due to localization.
	/// </remarks>
	bool ConnectBLEDeviceByName(const std::wregex& deviceNameRegex);
	/// <summary>
	/// Selects a GattDeviceService on which the notify and write property reside after being connected to a device.
	/// </summary>
	/// <param name="serviceGUIDRegex">
	/// A regular expression which should match the GUID of the service.
	/// The GUID is always of the form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}.
	/// </param>
	/// <returns>
	/// A boolean stating whether the function succeeded. Use GetLastError() for information about a failure.
	/// </returns>
	bool SelectService(const std::wregex& serviceGUIDRegex);
	/// <summary>
	/// Connects to a BluetoothLEDevice which has a GattDeviceService which's GUID matches the given regex.
	/// It will select the matched service.
	/// </summary>
	/// <param name="serviceGUIDRegex">
	/// A regular expression which should match the GUID of the service.
	/// The GUID is always of the form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}.
	/// </param>
	/// <returns>
	/// A boolean stating whether the function succeeded. Use GetLastError() for information about a failure.
	/// </returns>
	bool ConnectBLEDeviceByService(const std::wregex& serviceGUIDRegex);
	/// <summary>
	/// Selects GattCharacteristics with GattCharacteristicProperties Write and Notify.
	/// Will set the configuration descriptor of the Notify characteristic to Notify and adds a callback to ValueChanged.
	/// </summary>
	/// <param name="onValueChanged">
	/// The function that should be executed every time the device notifies.
	/// </param>
	/// <returns>
	/// A boolean stating whether the function succeeded. Use GetLastError() for information about a failure.
	/// </returns>
	bool SelectCharacteristics(const std::function<void(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic sender, winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs eventArgs)>& onValueChanged);
	/// <summary>
	/// Performs a characteristic value write to a device.
	/// </summary>
	/// <param name="cmd">
	/// The data that should be written to the device.
	/// </param>
	/// <returns>
	/// A boolean stating whether the function succeeded. Use GetLastError() for information about a failure.
	/// </returns>
	bool GattWrite(const std::wstring& cmd);
	/// <summary>
	/// Closes all connections and disconnects the device.
	/// </summary>
	/// <returns>
	/// A boolean stating whether the function succeeded. Use GetLastError() for information about a failure.
	/// </returns>
	bool DisconnectBLEDevice();
};