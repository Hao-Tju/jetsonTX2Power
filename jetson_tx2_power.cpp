#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

#include <cstdio>
#include <cstring>
#include <ctime>

#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "jetson_tx2_power.hh"

using namespace std;

void PowerReadingValue::update(void) {
  ifstream inFile;
  inFile.open(this->path.c_str());
  if (!inFile.is_open()) {
    this->value = "-1.0";
  } else {
    inFile >> this->value;
    inFile.close();
  }
}

std::string PowerReadingValue::get_name(void) { return this->name; }
std::double PowerReadingValue::get_value(void) { return this->value; }

PowerReadingValue::PowerReadingValue(std::string name) {
  this->name = name;
  this->path = "";
  this->value = 0.0;
  this->number = 0;
}

std::ostream &operator<<(std::ostream &stream, const PowerReadingValue &value) {
  stream << "name:" << value.name << ", path:" << value.path
         << ", value:" << value.value;
  return stream;
}

PowerReadingVoltage::PowerReadingVoltage(std::string path, int number)
    : PowerReadingValue("voltage") {
  std::ostringstream ss;
  ss << number;
  std::string s(ss.str());

  this->name = name;
  this->path = path + "/in_voltage" + s + "_input";
  this->value = 0.0;
}

PowerReadingCurrent::PowerReadingCurrent(std::string path, int number)
    : PowerReadingValue("current") {
  std::ostringstream ss;
  ss << number;
  std::string s(ss.str());

  this->name = name;
  this->path = path + "/in_current" + s + "_input";
  this->value = 0.0;
}

PowerReadingPower::PowerReadingPower(std::string path, int number)
    : PowerReadingValue("power") {
  ostringstream ss;
  ss << number;
  string s(ss.str());

  this->path = path + "/in_power" + s + "_input";
  this->value = 0.0;
}

PowerReadingRail::PowerReadingRail(std::string path, int num)
    : voltage(path, num), current(path, num) {
  std::ifstream inFile;

  std::ostringstream ss;
  ss << num;
  std::string s(ss.str());

  inFile.open((path + "/rail_name_" + s).c_str());
  if (!inFile) {
    this->name = "";
  } else {
    inFile >> this->name;
    inFile.close();
  }
}

std::string PowerReadingRail::get_name(void) { return this->name; }

void PowerReadingRail::update(void) {
  this->voltage.update();
  this->current.update();
  this->power.update();
}

std::string PowerReadingRail::to_csv(void) {
  string s = to_string(this->voltage.get_value()) + "," +
             to_string(this->current.get_value()) + "," +
             to_string(this->power.get_value());

  return s;
}

std::string PowerReadingRail::get_csv_header(void) {
  string s = this->name + " voltage," + this->name + " current," + this->name +
             " power";

  return s;
}

std::ostream &operator<<(std::ostream &stream, const PowerReadingRail &r) {
  stream << "name:" << r.name << endl;
  stream << "    voltage:" << r.voltage << endl;
  stream << "    current:" << r.current << endl;
  stream << "    power:" << r.power;

  return stream;
}

PowerReadingDevice::PowerReadingDevice(std::string path) {
  for (int i = 0; i < 3; ++i) {
    rails.push_back(PowerReadingRail(path, i));
  }
}

void PowerReadingDevice::update(void) {
  for_each (this->rails.begin(), this->rails.end(), [](PowerReadingRail& rail) { rail.update(); });
  // for (std::vector<PowerReadingRail>::iterator it = this->rails.begin();
  //      it != this->rails.end(); ++it, ++i) {
  //   it->update();
  // }
}

std::string PowerReadingDevice::to_csv(void) {
  std::string s;

  for (std::vector<PowerReadingRail>::iterator it = this->rails.begin();
       it != this->rails.end(); ++it) {
    s += it->to_csv();
    if (it != this->rails.end() - 1) {
      s += ",";
    }
  }

  return s;
}

std::string PowerReadingDevice::get_csv_header(void) {
  std::string s;
  for (std::vector<PowerReadingRail>::iterator it = this->rails.begin();
       it != this->rails.end(); ++it) {
    s += it->get_csv_header();
    if (it != this->rails.end() - 1) {
      s += ",";
    }
  }

  return s;
}

std::ostream &operator<<(std::ostream &stream, const PowerReadingDevice &d) {
  stream << "Rails:" << endl;

  for_each (d.rails.begin(), d.rails.end(), [](const PowerReadingRail& rail) { cout << rail << endl; });
  // for (std::vector<PowerReadingRail>::const_iterator it = d.rails.begin();
  //      it != d.rails.end(); ++it) {
  //   cout << *it << endl;
  // }
  return stream;
}

std::vector<PowerReadingDevice> create_devices(void) {
  std::vector<PowerReadingDevice> devices;

  devices.push_back(
      PowerReadingDevice("/sys/devices/3160000.i2c/i2c-0/0-0040/iio_device"));
  devices.push_back(
      PowerReadingDevice("/sys/devices/3160000.i2c/i2c-0/0-0041/iio_device"));
  // devices.push_back(
  //     PowerReadingDevice("/sys/devices/3160000.i2c/i2c-0/0-0042/iio_device"));
  // devices.push_back(
  //     PowerReadingDevice("/sys/devices/3160000.i2c/i2c-0/0-0043/iio_device"));

  return devices;
}

void to_csv(std::string csv_file, std::vector<PowerReadingDevice> &devices,
            std::map<std::string, std::string> &xtra_fields) {
  int header = 0;
  std::ofstream outFile;
  char buf[8192];
  struct timeval now;
  time_t t;
  struct tm *tmp;

  if (access(csv_file.c_str(), F_OK) == -1) {
    header = 1;
    outFile.open(csv_file.c_str());
  } else {
    outFile.open(csv_file.c_str(), std::ios_base::app);
  }

  if (!outFile.is_open()) {
    return;
  }

  if (header != 0) {
    outFile << "date,time,";

    for (vector<PowerReadingDevice>::iterator it = devices.begin();
         it != devices.end(); ++it) {
      outFile << it->get_csv_header();
      if (it < (devices.end() - 1)) {
        outFile << ",";
      }
    }

    if (!xtra_fields.empty()) {
      outFile << ",";
    }
    for (std::map<std::string, std::string>::iterator it = xtra_fields.begin();
         it != xtra_fields.end();) {
      outFile << it->first;
      if (++it != xtra_fields.end())
        outFile << ",";
    }
    outFile << endl;
  }

  update_power_values(devices);

  gettimeofday(&now, NULL);
  t = now.tv_sec;
  tmp = localtime(&t);

  memset(buf, '\0', sizeof(buf));
  strftime(buf, sizeof(buf), "%Y-%m-%d", tmp);
  outFile << buf;

  memset(buf, '\0', sizeof(buf));
  strftime(buf, sizeof(buf), ",%H:%M:%S", tmp);
  outFile << buf << ":" << (now.tv_usec / 1000) << ",";

  for (vector<PowerReadingDevice>::iterator it = devices.begin();
       it != devices.end(); ++it) {
    it->update();
    outFile << it->to_csv();
    if (it < (devices.end() - 1)) {
      outFile << ",";
    }
  }

  if (!xtra_fields.empty()) {
    outFile << ",";
  }
  for (std::map<std::string, std::string>::iterator it = xtra_fields.begin();
       it != xtra_fields.end();) {
    outFile << it->second;
    if (++it != xtra_fields.end())
      outFile << ",";
  }
  outFile << endl;

  outFile.close();
}

void to_csv(std::string csv_file, std::vector<PowerReadingDevice> &devices,
            std::string comment) {
  int header = 0;
  std::ofstream outFile;
  char buf[8192];
  struct timeval now;
  time_t t;
  struct tm *tmp;

  if (access(csv_file.c_str(), F_OK) == -1) {
    header = 1;
    outFile.open(csv_file.c_str());
  } else {
    outFile.open(csv_file.c_str(), std::ios_base::app);
  }

  if (!outFile.is_open()) {
    return;
  }

  if (header != 0) {
    outFile << "date,time,";

    for (vector<PowerReadingDevice>::iterator it = devices.begin();
         it != devices.end(); ++it) {
      outFile << it->get_csv_header();
      if (it != (devices.end() - 1) or
          comment.length() != 0) {
        outFile << ",";
      }
    }

    if (comment.length() != 0) {
      outFile << "comments" << endl;
    }
  }

  update_power_values(devices);

  gettimeofday(&now, NULL);
  t = now.tv_sec;
  tmp = localtime(&t);

  memset(buf, '\0', sizeof(buf));
  strftime(buf, sizeof(buf), "%Y-%m-%d", tmp);
  outFile << buf;

  memset(buf, '\0', sizeof(buf));
  strftime(buf, sizeof(buf), ",%H:%M:%S", tmp);
  outFile << buf << ":" << (now.tv_usec / 1000) << ",";

  for (vector<PowerReadingDevice>::iterator it = devices.begin();
       it != devices.end(); ++it) {
    it->update();
    outFile << it->to_csv();

    if (it != (devices.end() - 1) or
        comment.length() != 0) {
      outFile << ",";
    }
  }
  outFile << comment << endl;

  outFile.close();
}

void to_csv(std::string csv_file, std::vector<PowerReadingDevice> &devices) {
  to_csv(csv_file, devices, "");
}

int update_power_values(std::vector<PowerReadingDevice> &devices) {
  for_each (devices.begin(), devices.end(), [](PowerReadingDevice& dev_rail) { dev_rail.update(); });
  // for (std::vector<PowerReadingDevice>::iterator it = devices.begin();
  //      it != devices.end(); ++it) {
  //   it->update();
  // }

  return 0;
}

int print_values(std::vector<PowerReadingDevice> &devices) {
  time_t result;

  result = time(NULL);
  printf("%.19s\n", ctime(&result));

  for (std::vector<PowerReadingDevice>::iterator it = devices.begin();
       it != devices.end(); ++it) {
    it->update();
    cout << *it << endl;
  }
  return 0;
}
