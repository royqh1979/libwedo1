#Libwedo1

A C Library for Lego wedo 1.0 inspired by WedoMore (https://github.com/itdaniher/WeDoMore)

Licensed under Apache License 2.0

Depends on libusb-1.0 (http://libusb.info/)

Tested on mingw+gcc 4.7.2

###Support:
  * 9581 WeDo Robotics USB Hub 
  * 9583 WeDo Robotics Motion Sensor 
  * 9584 WeDo Robotics Tilt Sensor
  * 8883 Power Functions M-Motor 
  * 8870 Power Functions Light
  * 88003 Power Functions L-Motor
  * 88004 Power Functions Servo Motor
  
###Usage: (see src/test.c)
 
  initialization / close 
  * wedo_init();
  * wedo_close();
  
  for sensors:
  * wedo_get_tilt();
  * wedo_get_distance();
    
  for motor:
  
  * wedo_start_mortor(int power);
  * wedo_stop_mortor();
  
  for light:
  * wedo_light_on();
  * wedo_light_off();
  
  for motor_servo:
  * wedo_servo_rotate(int degree);
  * wedo_servo_off();
  
  
  
