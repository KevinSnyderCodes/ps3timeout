#include <linux/hidraw.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
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
 * - Controller disconnected by user: stop daemon logic for that device
 */

int main (int argc, char **argv)
{
  // Open device
  char *device = "/dev/hidraw0";
  int fd = open(device, O_RDONLY);
  cout << fd << endl;

  // Get physical address
  //
  // Unfortunately this gives us the MAC address of the bluetooth adapter,
  // not of the PS3 controller. So we will need to either:
  //
  // 1. Get the PS3 controller MAC address another way
  // 2. Find a different way entirely to disconnect out hidraw device,
  //    as long as it results in the controller turning off.
  //
  // The hidraw API does not offer an ioctl method to disconnect:
  // https://github.com/intel/edison-linux/blob/master/Documentation/hid/hidraw.txt
  //
  // The controller's MAC address can be found at:
  // /sys/class/hidraw/hidraw0/device/uevent
  // (label HID_UNIQ)
  char buf_addr[256];
  int res = ioctl(fd, HIDIOCGRAWPHYS(256), buf_addr);
  cout << res << endl;
  cout << buf_addr << endl;

  // Read from device
  unsigned char buf_old[128];
  while (true)
  {
    unsigned char buf_read[128];
    int nr = read(fd, buf_read, sizeof(buf_read));
    cout << nr << endl;
    if (nr == 49)
    {
      for (int i = 50; i > 0; i--)
      {
        buf_read[i] = buf_read[i-1];
      }
    }
    // Output changed values
    for (int i = 0; i < 128; i++)
    {
      if (buf_read[i] != buf_old[i])
      {
        // cout << "Value " << i << " changed: " << (int)buf_read[i] << endl;
      }
    }
    cout << (int)buf_read[15] << endl;
    memcpy(buf_old, buf_read, 128*sizeof(char));
    // Don't use usleep! Causes latest buf_read values to be delayed.
    // usleep(1000000);
  }
}
