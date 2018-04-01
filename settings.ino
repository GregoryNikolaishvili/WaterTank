const byte DEF_SETTINGS_VERSION = 0x00;
const int STORAGE_ADDRESS_SETTINGS = 0;

extern SettingStructure settings[TANK_COUNT];

void readSettings()
{
	settings[0].MaxDistance = 200;
	settings[0].MinDistance = 15;
	settings[1].MaxDistance = 250;
	settings[1].MinDistance = 20;
	settings[2].MaxDistance = 250;
	settings[2].MinDistance = 20;

	byte v = eeprom_read_byte((uint8_t *)STORAGE_ADDRESS_SETTINGS);
	if (v != DEF_SETTINGS_VERSION)
	{
		eeprom_update_byte((uint8_t *)STORAGE_ADDRESS_SETTINGS, DEF_SETTINGS_VERSION);
		saveSettings(false);
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

