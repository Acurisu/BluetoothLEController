// Copyright 2021, Acurisu
// Author: Acurisu
// E-Mail: acurisu@gmail.com
// Licensed under the MIT License.

#include "BluetoothLEController.hpp"

bool BluetoothLEController::IsWatcherStarted(const winrt::Windows::Devices::Enumeration::DeviceWatcher& watcher)
{
	winrt::Windows::Devices::Enumeration::DeviceWatcherStatus status = watcher.Status();
	return (status == winrt::Windows::Devices::Enumeration::DeviceWatcherStatus::Started) || (status == winrt::Windows::Devices::Enumeration::DeviceWatcherStatus::EnumerationCompleted);
}

bool BluetoothLEController::LookupBooleanProperty(const winrt::Windows::Devices::Enumeration::DeviceInformation& deviceInfo, winrt::hstring const& property)
{
	auto value = deviceInfo.Properties().TryLookup(property);
	return value && winrt::unbox_value<bool>(value);
}

bool BluetoothLEController::HasFlag(uint32_t value, uint32_t flags)
{
	return (value & flags) == flags;
}

bool BluetoothLEController::BLEDeviceExistsAndConnected()
{
	if (bluetoothLEDevice == nullptr)
	{
		SetLastError(BluetoothLEControllerErrorCodes::BluetoothLEDeviceNotFound);
		return false;
	}

	if (bluetoothLEDevice.ConnectionStatus() == winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus::Disconnected)
	{
		SetLastError(BluetoothLEControllerErrorCodes::BluetoothLEDeviceDisconnected);
		return false;
	}

	return true;
}

bool BluetoothLEController::SelectBLEDeviceService(const winrt::Windows::Devices::Bluetooth::BluetoothLEDevice& device, const std::wregex& serviceGUIDRegex)
{
	auto services = device.GetGattServicesAsync(winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get().Services();
	for (winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService service : services)
	{
		if (std::regex_match(to_hstring(service.Uuid()).c_str(), serviceGUIDRegex))
		{
			gattDeviceService = service;
			return true;
		}
	}
	return false;
}

bool BluetoothLEController::SelectBLEDevice(const winrt::Windows::Devices::Enumeration::DeviceInformation& deviceInfo, const std::wregex& regex, Selector selector)
{
	if (LookupBooleanProperty(deviceInfo, isConnectable))
	{
		switch (selector)
		{
		case Selector::byDeviceID:
		case Selector::byDeviceName:
			if (std::regex_match((selector == Selector::byDeviceID ? deviceInfo.Id() : deviceInfo.Name()).c_str(), regex))
			{
				bluetoothLEDevice = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromIdAsync(deviceInfo.Id()).get();
				return true;
			}
			break;

		case Selector::byServiceGUID:
			winrt::Windows::Devices::Bluetooth::BluetoothLEDevice device = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromIdAsync(deviceInfo.Id()).get();
			if (SelectBLEDeviceService(device, regex))
			{
				bluetoothLEDevice = device;
				return true;
			}
			break;
		}
	}

	return false;
}

bool BluetoothLEController::ConnectBLEDevice(const std::wregex& regex, Selector selector)
{
	std::mutex m;
	std::condition_variable cv;

	winrt::Windows::Devices::Enumeration::DeviceWatcher deviceWatcher = winrt::Windows::Devices::Enumeration::DeviceInformation::CreateWatcher(
		L"(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")",
		winrt::single_threaded_vector<winrt::hstring>({ L"System.Devices.Aep.DeviceAddress", isConnected, isConnectable }),
		winrt::Windows::Devices::Enumeration::DeviceInformationKind::AssociationEndpoint);

	std::unordered_map<winrt::hstring, winrt::Windows::Devices::Enumeration::DeviceInformation> map;
	winrt::event_token added = deviceWatcher.Added([this, &regex, &selector, &cv, &deviceWatcher, &map](const winrt::Windows::Devices::Enumeration::DeviceWatcher& sender, const winrt::Windows::Devices::Enumeration::DeviceInformation& deviceInfo)
	{
		if (deviceWatcher == sender && IsWatcherStarted(sender))
		{
			if (!deviceInfo.Name().empty())
			{
				map.insert(std::make_pair(deviceInfo.Id(), deviceInfo));
				if (SelectBLEDevice(deviceInfo, regex, selector))
				{
					deviceWatcher.Stop();
				}
			}
		}
		else
		{
			cv.notify_all();
		}
	});

	winrt::event_token updated = deviceWatcher.Updated([this, &regex, &selector, &cv, &deviceWatcher, &map](const winrt::Windows::Devices::Enumeration::DeviceWatcher& sender, const winrt::Windows::Devices::Enumeration::DeviceInformationUpdate& deviceInfoUpdate)
	{
		if (deviceWatcher == sender && IsWatcherStarted(sender))
		{
			auto it = map.find(deviceInfoUpdate.Id());
			if (it != map.end())
			{
				it->second.Update(deviceInfoUpdate);
				if (SelectBLEDevice(it->second, regex, selector))
				{
					deviceWatcher.Stop();
				}
			}
		}
		else
		{
			cv.notify_all();
		}
	});


	winrt::event_token removed = deviceWatcher.Removed([&cv, &deviceWatcher, &map](const winrt::Windows::Devices::Enumeration::DeviceWatcher& sender, const winrt::Windows::Devices::Enumeration::DeviceInformationUpdate& deviceInfoUpdate)
	{
		if (deviceWatcher == sender && IsWatcherStarted(sender))
		{
			auto it = map.find(deviceInfoUpdate.Id());
			if (it != map.end())
			{
				map.erase(it);
			}
		}
		else
		{
			cv.notify_all();
		}
	});

	winrt::event_token stopped = deviceWatcher.Stopped([&cv](const winrt::Windows::Devices::Enumeration::DeviceWatcher& sender, const winrt::Windows::Foundation::IInspectable&)
	{
		cv.notify_all();
	});

	std::unique_lock l(m);

	deviceWatcher.Start();
	cv.wait(l, [&deviceWatcher]
	{
		return !IsWatcherStarted(deviceWatcher);
	});

	deviceWatcher.Added(added);
	deviceWatcher.Updated(updated);
	deviceWatcher.Removed(removed);
	deviceWatcher.Stopped(stopped);

	if (bluetoothLEDevice == nullptr)
	{
		switch (selector)
		{
		case Selector::byDeviceID:
		case Selector::byDeviceName:
			SetLastError(BluetoothLEControllerErrorCodes::BluetoothLEDeviceNotFound);
			break;

		case Selector::byServiceGUID:
			SetLastError(BluetoothLEControllerErrorCodes::GattDeviceServiceNotFound);
			break;
		}

		return false;
	}

	return true;
}

bool BluetoothLEController::ConnectBLEDeviceByID(const std::wregex& deviceIDRegex)
{
	return ConnectBLEDevice(deviceIDRegex, Selector::byDeviceID);
}

bool BluetoothLEController::ConnectBLEDeviceByName(const std::wregex& deviceNameRegex)
{
	return ConnectBLEDevice(deviceNameRegex, Selector::byDeviceName);
}

bool BluetoothLEController::SelectService(const std::wregex& serviceGUIDRegex)
{
	if (bluetoothLEDevice == nullptr)
	{
		SetLastError(BluetoothLEControllerErrorCodes::BluetoothLEDeviceNotFound);
		return false;
	}

	if (!SelectBLEDeviceService(bluetoothLEDevice, serviceGUIDRegex))
	{
		SetLastError(BluetoothLEControllerErrorCodes::GattDeviceServiceNotFound);
		return false;
	}

	if (bluetoothLEDevice.ConnectionStatus() == winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus::Disconnected)
	{
		SetLastError(BluetoothLEControllerErrorCodes::BluetoothLEDeviceDisconnected);
		return false;
	}

	return true;
}

bool BluetoothLEController::ConnectBLEDeviceByService(const std::wregex& serviceGUIDRegex)
{
	if (!ConnectBLEDevice(serviceGUIDRegex, Selector::byServiceGUID))
	{
		return false;
	}

	if (bluetoothLEDevice.ConnectionStatus() == winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus::Disconnected)
	{
		SetLastError(BluetoothLEControllerErrorCodes::BluetoothLEDeviceDisconnected);
		return false;
	}

	return true;
}

bool BluetoothLEController::SelectCharacteristics(const std::function<void(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic sender, winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs eventArgs)>& onValueChanged)
{
	if (!BLEDeviceExistsAndConnected()) {
		return false;
	}

	if (gattDeviceService == nullptr)
	{
		SetLastError(BluetoothLEControllerErrorCodes::GattDeviceServiceNotFound);
		return false;
	}

	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicsResult gattCharacteristicsResult = gattDeviceService.GetCharacteristicsAsync(winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get();
	if (gattCharacteristicsResult.Status() != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
	{
		SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::NotifyPropertyNotFound + static_cast<DWORD>(gattCharacteristicsResult.Status()));
		return false;
	}

	for (winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic characteristic : gattCharacteristicsResult.Characteristics())
	{
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties properties = characteristic.CharacteristicProperties();
		if (HasFlag(static_cast<uint32_t>(properties), static_cast<uint32_t>(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties::Write)))
		{
			gattCharacteristicWrite = characteristic;
		}
		else if (HasFlag(static_cast<uint32_t>(properties), static_cast<uint32_t>(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties::Notify)))
		{
			winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus status = characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::Notify).get();
			if (status != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
			{
				SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::NotifyPropertyNotFound + static_cast<DWORD>(status));
				return false;
			}

			characteristic.ValueChanged(onValueChanged);

			gattCharacteristicNotify = characteristic;
		}
	}

	if (gattCharacteristicWrite == nullptr || gattCharacteristicNotify == nullptr)
	{
		if (gattCharacteristicWrite == nullptr && gattCharacteristicNotify == nullptr)
		{
			SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::PropertiesNotFound);
		}
		else if (gattCharacteristicWrite == nullptr)
		{
			SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::WritePropertyNotFound);
		}
		else
		{
			SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::NotifyPropertyNotFound);
		}

		return false;
	}

	return true;
}

bool BluetoothLEController::GattWrite(const std::wstring& cmd)
{
	if (!BLEDeviceExistsAndConnected()) {
		return false;
	}

	if (gattCharacteristicWrite == nullptr)
	{
		SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::WritePropertyNotFound);
		return false;
	}

	winrt::Windows::Storage::Streams::DataWriter dataWriter;
	dataWriter.WriteString(cmd);

	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus status = gattCharacteristicWrite.WriteValueWithResultAsync(dataWriter.DetachBuffer()).get().Status();
	if (status != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
	{
		SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::NotifyPropertyNotFound + static_cast<DWORD>(status));
		return false;
	}

	return true;
}

bool BluetoothLEController::DisconnectBLEDevice()
{
	if (bluetoothLEDevice != nullptr)
	{
		if (bluetoothLEDevice.ConnectionStatus() == winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus::Disconnected)
		{
			return true;
		}

		bluetoothLEDevice.Close();
		bluetoothLEDevice = nullptr;
	}

	if (gattCharacteristicNotify != nullptr)
	{
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus status = gattCharacteristicNotify.WriteClientCharacteristicConfigurationDescriptorAsync(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue::None).get();

		if (status != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success)
		{
			SetLastError(BluetoothLEControllerErrorCodes::GattCharacteristics::NotifyPropertyNotFound + static_cast<DWORD>(status));
			return false;
		}
	}

	if (gattDeviceService != nullptr)
	{
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession gattSession = gattDeviceService.Session();
		if (gattSession.SessionStatus() == winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSessionStatus::Active)
		{
			gattSession.Close();
		}
		gattDeviceService.Close();
		gattDeviceService = nullptr;
	}

	return true;
}