#include "idle.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>

IdlePlatform::IdlePlatform()
{
}


IdlePlatform::~IdlePlatform()
{
}


bool IdlePlatform::init()
{
  if (secondsIdle() == -1)
    return false;

  return true;
}


int IdlePlatform::secondsIdle()
{
  mach_port_t masterPort;
  io_iterator_t iter;
  io_registry_entry_t curObj;

  IOMasterPort(MACH_PORT_NULL, &masterPort);
  IOServiceGetMatchingServices(masterPort, IOServiceMatching("IOHIDSystem"), &iter);
  if (iter == 0)
    return -1;

  curObj = IOIteratorNext(iter);
  if (curObj == 0)
    return -1;

  CFMutableDictionaryRef properties = 0;
  CFTypeRef obj;
  int result = -1;

  if (IORegistryEntryCreateCFProperties(curObj, &properties, kCFAllocatorDefault, 0) == KERN_SUCCESS && properties != NULL) {
    obj = CFDictionaryGetValue(properties, CFSTR("HIDIdleTime"));
    CFRetain(obj);
  } else
    obj = NULL;

  if (obj) {
    uint64_t tHandle;

    CFTypeID type = CFGetTypeID(obj);

    if (type == CFDataGetTypeID())
      CFDataGetBytes((CFDataRef) obj, CFRangeMake(0, sizeof(tHandle)), (UInt8*) &tHandle);
    else if (type == CFNumberGetTypeID())
      CFNumberGetValue((CFNumberRef)obj, kCFNumberSInt64Type, &tHandle);
    else
      return -1;

    CFRelease(obj);

    // essentially divides by 10^9
    tHandle >>= 30;
    result = tHandle;
  }

  /* Release our resources */
  IOObjectRelease(curObj);
  IOObjectRelease(iter);
  CFRelease((CFTypeRef)properties);
  return result;
}