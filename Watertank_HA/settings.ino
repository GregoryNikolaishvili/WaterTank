const byte DEF_SETTINGS_VERSION = 0x00;
const int STORAGE_ADDRESS_SETTINGS = 0;

MinMaxVoltageSettingStructure tankVoltageSettings;

static void readSettings()
{
	// Big tank
	tankVoltageSettings.empty = 580; //mv
	tankVoltageSettings.full = 1050; //mv
	//// Small tank
	//tankVoltageSettings[1].empty = 520; //mv
	//tankVoltageSettings[1].full = 1000; //mv

	byte v = eeprom_read_byte((uint8_t*)STORAGE_ADDRESS_SETTINGS);
	if (v != DEF_SETTINGS_VERSION)
	{
		eeprom_update_byte((uint8_t*)STORAGE_ADDRESS_SETTINGS, DEF_SETTINGS_VERSION);
		saveSettings();
	}
	else
	{
		eeprom_read_block((void*)&tankVoltageSettings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(tankVoltageSettings));
	}

	settingsChanged(false);
}

static void saveSettings()
{
	eeprom_update_block((const void*)&tankVoltageSettings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(tankVoltageSettings));
}

static void settingsChanged(bool publish)
{
	//TODO	
}
