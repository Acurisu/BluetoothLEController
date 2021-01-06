// Copyright 2021, Acurisu
// Author: Acurisu
// E-Mail: acurisu@gmail.com
// Licensed under the MIT License.

#include <iostream>

#include "BluetoothLEController.hpp"

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