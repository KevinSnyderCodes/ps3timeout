#include <linux/hidraw.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <libudev.h>
#include <cstdlib>
#include <ctime>
#include <regex>
#include <fstream>
#include <iostream>
#include <thread>
#include <map>
using namespace std;

/**
 * LOGIC
 * 1. Get list of hidraw devices
 * 2. Get MAC address of devices from /sys/class/hidraw/hidrawX/device/uevent
 *    (label HID_UNIQ)
 * 3. "hcitool name ADDR" to make sure it's a PS3 controller
 * 4. Use buffer from read() to check for inputs
 * 5. If no input arrives after X amount of time, "hcitool dc ADDR"
 *
 * GOTCHAS
 * - New controller connected
 *   libudev has a monitoring interface: http://www.signal11.us/oss/udev/
 * - Controller(s) already connected when daemon started
 * - Controller disconnected by user: stop daemon logic for that device
 */

const int TIMEOUT_SECONDS = 15;

map<string, bool> device_connected;

struct stick {
  int x;
  int y;
};

struct controller {
  // b values are >1 when any face button (except the PS
  // button), shoulder button, or stick is pressed/clicked.
  int b1;
  int b2;
  int b3;
  // l and r are the position of the left and right stick.
  stick l;
  stick r;
  // These values are pressure sensitive.
  int up;
  int right;
  int down;
  int left;
  int l2;
  int r2;
  int l1;
  int r1;
  int triangle;
  int circle;
  int cross;
  int square;

  controller(unsigned char buf[128], int deadzone = 0)
  {
    this->b1 = buf[3];
    this->b2 = buf[4];
    this->b3 = buf[5];
    this->l.x = buf[7] - 128;
    this->l.y = buf[8] - 128;
    this->r.x = buf[9] - 128;
    this->r.y = buf[10] - 128;
    this->up = buf[15];
    this->right = buf[16];
    this->down = buf[17];
    this->left = buf[18];
    this->l2 = buf[19];
    this->r2 = buf[20];
    this->l1 = buf[21];
    this->r1 = buf[22];
    this->triangle = buf[23];
    this->circle = buf[24];
    this->cross = buf[25];
    this->square = buf[26];
    // Adjust for deadzone
    if (inDeadzone(this->l, deadzone))
    {
      this->l.x = 0;
      this->l.y = 0;
    }
    if (inDeadzone(this->r, deadzone))
    {
      this->r.x = 0;
      this->r.y = 0;
    }
  }
  bool isActive()
  {
    return this->b1 != 0 || this->b2 != 0 || this->b3 != 0 ||
           this->l.x != 0 || this->l.y != 0 ||
           this->r.x != 0 || this->r.y != 0;
  }
private:
  bool inRange(int val, int min, int max)
  {
    if (min > max)
    {
      swap(min, max);
    }
    return min < val && val < max;
  }
  bool inRange(int val, int range)
  {
    range = abs(range);
    return inRange(val, -range, range);
  }
  bool inDeadzone(stick s, int deadzone)
  {
    return inRange(s.x, deadzone) && inRange(s.y, deadzone);
  }
};

void watch_controller(string node)
{
  cout << node << endl;
  // Get device name
  regex r_device("^\\/dev\\/(.*)$");
  smatch m_device;
  regex_match(node, m_device, r_device);
  string device = m_device.str(1);
  cout << device << endl;

  // Get MAC address
  ifstream f("/sys/class/hidraw/" + device + "/device/uevent");
  if (!f.good())
  {
    cout << "Error opening uevent file!" << endl;
    return;
  }
  stringstream buf_file;
  buf_file << f.rdbuf();
  regex r_addr("(^|\\n)HID_UNIQ=(.*)(\\n|$)");
  smatch m_addr;
  string uevent = buf_file.str();
  if (!regex_search(uevent, m_addr, r_addr))
  {
    cout << "MAC address not found for device " << device << endl;
    return;
  }
  string addr = m_addr.str(2);

  // Check if device is PS3 controller
  char buf_cmd[128];
  string cmd_out = "";
  string cmd = "hcitool name " + addr;
  cout << cmd << endl;
  shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe)
  {
    cout << "Failed to pipe command output" << endl;
    return;
  }
  while (!feof(pipe.get()))
  {
    if (fgets(buf_cmd, 128, pipe.get()) != nullptr)
    {
      cmd_out += buf_cmd;
    }
  }
  cout << cmd_out << cmd_out.length() << endl;
  if (cmd_out != "PLAYSTATION(R)3 Controller\n")
  {
    cout << "Device " << device << " is not a PS3 controller" << endl;
    return;
  }

  // Open device
  int fd = open(node.c_str(), O_RDONLY);
  cout << fd << endl;

  // Check for activity
  long lastActive = time(0);
  while (true)
  {
    if (!device_connected[node])
    {
      return;
    }
    unsigned char buf_read[128];
    int nr = read(fd, buf_read, sizeof(buf_read));
    if (nr == 49)
    {
      for (int i = 50; i > 0; i--)
      {
        buf_read[i] = buf_read[i-1];
      }
    }
    controller hid(buf_read, 10);
    if (hid.isActive())
    {
      lastActive = time(0);
    }
    else if (lastActive + TIMEOUT_SECONDS < time(0))
    {
      cout << "Timeout!" << endl;
      string cmd = "hcitool dc " + addr;
      system(cmd.c_str());
      return;
    }
    cout << "watch_controller" << endl;
    usleep(1000*1000);
  }
}

int main (int argc, char **argv)
{
  // Set timeout
  int timeout = 60;
  if (argc > 1)
  {
    timeout = atoi(argv[1]);
  }
  cout << timeout << endl;

  // Get hidraw devices
  struct udev *udev = udev_new();
  if (!udev)
  {
    cout << "Can't create udev" << endl;
  }
  struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(mon, "hidraw", NULL);
  udev_monitor_enable_receiving(mon);
  int fd_mon = udev_monitor_get_fd(mon);

  while (true)
  {
    fd_set fds;
    struct timeval tv;
    int ret;

    FD_ZERO(&fds);
    FD_SET(fd_mon, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(fd_mon+1, &fds, NULL, NULL, &tv);

    if (ret > 0 && FD_ISSET(fd_mon, &fds))
    {
      struct udev_device *dev = udev_monitor_receive_device(mon);
      if (dev)
      {
        string node = udev_device_get_devnode(dev);
        string subsystem = udev_device_get_subsystem(dev);
        string action = udev_device_get_action(dev);
        cout << "Node: " << node << endl;
        cout << "Subsystem: " << subsystem << endl;
        cout << "Action: " << action << endl;
        if (action == "add")
        {
          device_connected[node] = true;
          thread(watch_controller, node).detach();
        }
        else if (action == "remove")
        {
          device_connected[node] = false;
        }
        udev_device_unref(dev);
      }
    }
    cout << "main" << endl;
    usleep(1000*1000);
  }

  return 0;

}
