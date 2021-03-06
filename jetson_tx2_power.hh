#ifndef _JETSON_TX2_POWER_HH_
#define _JETSON_TX2_POWER_HH_

#if __cplusplus<201103L
#error This C++ compiler does not support C++11 standard
#endif

#include <iostream>
#include <map>
#include <string>
#include <vector>

class PowerReadingValue {
protected:
  std::string name;
  std::string path;  // Path to the monitor device.
  std::double value; // The current value of the monitor.
  int number;

public:
  PowerReadingValue(std::string name);
  void update(void);
  std::string get_name(void);
  std::double get_value(void);
  friend std::ostream &operator<<(std::ostream &stream,
                                  const PowerReadingValue &value);
};

class PowerReadingVoltage : public PowerReadingValue {
public:
  PowerReadingVoltage(std::string path, int number);
};

class PowerReadingCurrent : public PowerReadingValue {
public:
  PowerReadingCurrent(std::string path, int number);
};

class PowerReadingPower : public PowerReadingValue {
public:
  PowerReadingPower(std::string path, int number);
}

class PowerReadingRail {
protected:
  std::string name;
  PowerReadingVoltage voltage;
  PowerReadingCurrent current;
  PowerReadingPower power;

public:
  PowerReadingRail(std::string path, int num);
  std::string get_name(void);
  void update(void);
  std::string to_csv(void);
  std::string get_csv_header(void);
  friend std::ostream &operator<<(std::ostream &stream,
                                  const PowerReadingRail &r);
};

class PowerReadingDevice {
protected:
  std::vector<PowerReadingRail> rails;

public:
  PowerReadingDevice(std::string path);
  void update(void);
  std::string to_csv(void);
  std::string get_csv_header(void);
  friend std::ostream &operator<<(std::ostream &stream,
                                  const PowerReadingDevice &d);

};

std::vector<PowerReadingDevice> create_devices(void);
void to_csv(std::string csv_file, std::vector<PowerReadingDevice> &devices,
            std::string comment);
void to_csv(std::string csv_file, std::vector<PowerReadingDevice> &devices);
void to_csv(std::string csv_file, std::vector<PowerReadingDevice> &devices,
            std::map<std::string, std::string> &xtra_fields);
int update_power_values(std::vector<PowerReadingDevice> &devices);
int print_values(std::vector<PowerReadingDevice> &devices);

#endif // _JETSON_TX2_POWER_HH_
