const byte DEF_SETTINGS_VERSION = 0x02;
const int STORAGE_ADDRESS_SETTINGS = 0;
//const int STORAGE_ADDRESS_DATA = 100;


extern SettingStructure settings[TANK_COUNT];

void readSettings()
{
	settings[0].MaxDistance = 150;
	settings[0].MinDistance = 20;
	settings[1].MaxDistance = 160;
	settings[1].MinDistance = 20;
	settings[2].MaxDistance = 160;
	settings[2].MinDistance = 20;

	byte v = eeprom_read_byte((uint8_t *)STORAGE_ADDRESS_SETTINGS);
	if (v != DEF_SETTINGS_VERSION)
	{
		eeprom_update_byte((uint8_t *)STORAGE_ADDRESS_SETTINGS, DEF_SETTINGS_VERSION);
		saveSettings(false);

		//saveData("000C0;1;2;3;4;5;", 16);
	}
	else
	{
		eeprom_read_block((void*)&settings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(settings));
		settingsChanged(false);
	}
}

void saveSettings(bool publish)
{
	eeprom_update_block((const void*)&settings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(settings));

	settingsChanged(publish);
}

void settingsChanged(bool publish)
{
	recalcWaterLevelPercents();

	if (publish)
		PublishSettings();
}

//void saveData(const void* data, int length)
//{
//	if (length > 256)
//		length = 256;
//
//	eeprom_update_word((uint16_t *)STORAGE_ADDRESS_DATA, length);
//	eeprom_update_block(data, (void*)(STORAGE_ADDRESS_DATA + 2), length);
//}
