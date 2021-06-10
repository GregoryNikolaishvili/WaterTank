const byte DEF_SETTINGS_VERSION = 0x01;
const int STORAGE_ADDRESS_SETTINGS = 0;


void readSettings()
{
	maxDepthSettings[0] = 110; //cm
	maxDepthSettings[1] = 150; //cm

	byte v = eeprom_read_byte((uint8_t*)STORAGE_ADDRESS_SETTINGS);
	if (v != DEF_SETTINGS_VERSION)
	{
		eeprom_update_byte((uint8_t*)STORAGE_ADDRESS_SETTINGS, DEF_SETTINGS_VERSION);
		saveSettings(false);
	}
	else
	{
		eeprom_read_block((void*)&maxDepthSettings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(maxDepthSettings));
		settingsChanged(false);
	}
}

void saveSettings(bool publish)
{
	eeprom_update_block((const void*)&maxDepthSettings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(maxDepthSettings));

	settingsChanged(publish);
}

void settingsChanged(bool publish)
{
	recalcWaterLevelPercents();

	if (publish)
		PublishSettings();
}
