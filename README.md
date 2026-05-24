# Sensors To Home Assistant (or mqtt)

This usermod will send data to `grafana.local` on `8086` directly into the influxDB

# Installation

instructions < 16.0.0

## Enable in WLED

1. Edit `usermod_v2_DataToInfluxDB.h` to fit your needs

2. Add to `usermods_list.cpp`

add above register:
```
  #ifdef USERMOD_DATATOINFLUXDB
  #include "../usermods/usermod_v2_data_to_influxdb/usermod_v2_DataToInfluxDB.h"
  #endif
  
```

end of file within the register function
```
  #ifdef USERMOD_DATATOINFLUXDB
  usermods.add(new UserMod_DataToInfluxDB());
  #endif
```

1. Add to `build_flags` in platformio.ini:

```
  -D USERMOD_DATATOINFLUXDB
```

# Credits

- Aircoookie for making WLED
- Other usermod creators for example code
- You, for reading this
