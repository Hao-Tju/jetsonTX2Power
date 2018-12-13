#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "jetson_tx2_power.hh"

using namespace std;

template <typename T, size_t N>
T *end(T (&ra)[N]) { return ra + N; }

static int running;
void handle_sigint(int signal) { running = 0; }

void ParseArgs(int argc, char *argv[], char *csv_filename, int& msecs, struct timespec& delay, int& loops)
{
  delay.tv_sec = delay.tv_nsec = 0;
  loops = 0;

  int arg;
  while ((arg = getopt(argc, argv, "hf:c:t:")) != -1) {
    switch (arg) {
    case 'f':
      csv_filename = optarg;
      break;
    case 't':
      msecs = atoi(optarg);
      delay.tv_sec = msecs / 1000;
      delay.tv_nsec = (msecs % 1000) * 1000000;
      break;
    case 'c':
      loops = atoi(optarg);
      break;
    case 'h':
      cout << "Reads the power information from the INA3221 device on the "
        "Jetson TX2 board." << endl;
      cout << "-f <csv file>\t\tWrites to a CSV file. If the file already exists "
        "the values will be appended at the end." << endl;
      cout << "-c <number>\t\tNumber of probing iterations." << endl;
      cout << "-t <period>\t\tThe sample interval (milliseconds). An value higher "
        "than 1000 (i.e., 1s) is recommended." << endl;
      return 0;
    case '?':
      if (optopt == 'f' or optopt == 'c' or optopt == 't') {
        cerr << "option -" << optopt << "requires an argument." << endl;
      } else if (isprint(optopt)) {
        cerr << "Unknown option '-" << optopt << "\'." << endl;
      } else {
        cerr << "Unknown option character '\\x" << optopt << "\'." << endl;
      }
      return 1;
    default:
      abort();
    }
  }
}

int main(int argc, char *argv[]) {
  char *csv_filename = NULL;
  int msecs = 0, loops = 0;
  struct timespec delay;

  running = 1;
  opterr = 0;

  // Parse command arguments.
  ParseArgs(argc, argv, csv_filename, msecs, delay, loops);

  std::vector<PowerReadingDevice> devices = create_devices();
  if (msecs == 0 && loops == 0) {
    if (csv_filename == NULL)
      print_values(devices);
    else
      to_csv(csv_filename, devices);
  }
  if (msecs != 0 && loops == 0) {
    signal(SIGINT, handle_sigint);
    while (running == 1) {
      if (csv_filename == NULL)
        print_values(devices);
      else
        to_csv(csv_filename, devices);

      nanosleep(&delay, NULL);
    }
  }
  if (msecs == 0 && loops != 0) {
    for (c = 0; c < loops; c++) {
      if (csv_filename == NULL)
        print_values(devices);
      else
        to_csv(csv_filename, devices);
    }
  } else {
    for (c = 0; c < loops; c++) {
      if (csv_filename == NULL)
        print_values(devices);
      else
        to_csv(csv_filename, devices);
      nanosleep(&delay, NULL);
    }
  }

  return (0);
}
