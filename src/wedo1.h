//
// Created by Roy on 2017/7/7.
//

#ifndef LIBWEDO1_WEDO1_H
#define LIBWEDO1_WEDO1_H

#ifdef __cplusplus
extern "C" {
#endif

/* tilt type */
enum {
    WEDO_TILT_NONE=0,
    WEDO_TILT_FORWARD=0x1,
    WEDO_TILT_BACK=0x2,
    WEDO_TILT_LEFT=0x4,
    WEDO_TILT_RIGHT=0x8
};
#define WEDO_TILT_ALL (WEDO_TILT_FRONT | WEDO_TILT_BACK | WEDO_TILT_LEFT | WEDO_TILT_RIGHT)

/**
 * Initiatize Wedo 1.0
 *  it can only run once in one program each
 * @return init result:
 *  0 success
 *  <0 fail
 */
int wedo_init();
/**
 * close & cleanup Wedo 1.0
 */
void wedo_close();

/**
 * start wedo motors
 * @param power  motor power
 *      0    - stop motor
 *      50   - half power & turn clockwise
 *      100  - full power & turn clockwise
 *      -50  - half power & turn counter-clockwise
 *      -100 - full power & turn counter-clockwise
 * @return 0 success, <0 fail
 */
int wedo_start_mortor(int power);
/**
 * stop wedo motors
 * @return 0 success, <0 fail
 */
int wedo_stop_mortor();
/**
 * start wedo motor on slot a
 * @param power  motor power
 *      0    - stop motor
 *      50   - half power & turn clockwise
 *      100  - full power & turn clockwise
 *      -50  - half power & turn counter-clockwise
 *      -100 - full power & turn counter-clockwise
 * @return 0 success, <0 fail
 */
int wedo_start_mortor_a(int power);
/**
 * stop wedo motor on slot a
 * @return 0 success, <0 fail
 */
int wedo_stop_mortor_a();
/**
 * start wedo motor on slob b
 * @param power  motor power
 *      0    - stop motor
 *      50   - half power & turn clockwise
 *      100  - full power & turn clockwise
 *      -50  - half power & turn counter-clockwise
 *      -100 - full power & turn counter-clockwise
 * @return 0 success, <0 fail
 */
int wedo_start_mortor_b(int power);
/**
 * stop wedo motor on slot b
 * @return 0 success, <0 fail
 */
int wedo_stop_mortor_b();

int wedo_get_tilt();
int wedo_get_distance();

/*
int wedo_run_mortor(int power,int second);
int wedo_run_mortor_until_tilt();
int wedo_run_mortor_until_distance();
void wedo_wait_till_tilt();
void wedo_wait(int second);
void wedo_wait_till_distance();
 */

#ifdef __cplusplus
};
#endif

#endif //LIBWEDO1_WEDO1_H
