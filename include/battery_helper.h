// Helper declarations for battery INA226 setup
#pragma once

#include "INA226.h"

void setupBatteryINA(INA226& ina, unsigned int read_interval, const char* voltage_path,
                     const char* current_path, const char* chip_name);
