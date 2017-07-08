//
// Created by Roy on 2017/7/7.
//
#include <libusb-1.0/libusb.h>
#include "wedo1.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <stdbool.h>

#define ID_VENDOR 0x0694
#define ID_PRODUCT 0x0003
#define WEDO_CONFIGURATION 1

const int TILT_SENSORS[] = {38, 39, -1};
const int DISTANCE_SENSORS[] = {176, 177, 178, 179, -1};
const int MOTORS[] = {238,231,237,239,-1};
const int SERVO_MOTORS[] = {103,102,-1};
const int LIGHTS[] = {203,204,-1};
enum connect_type{
    TILT_SENSOR,
    DISTANCE_SENSOR,
    MOTOR,
    LIGHT,
    SERVO_MORTOR,
    NONE_CONNECT
};
typedef struct {
    struct libusb_device_handle *pHub;
    struct libusb_device *pHubDevice;
    int powerA;
    int powerB;
    enum connect_type A_connection_type;
    enum connect_type B_connection_type;
    volatile int distance;
    volatile int tilt;
    HANDLE hPollThread;
    struct libusb_transfer* pTransfer;
    unsigned char buffer[100];
} WEDO_HANDLE;
static WEDO_HANDLE *pWedo = NULL;
static volatile int do_exit=0;

static void __wedo_check_powers();

static void __wedo_init_data() {
    pWedo = (WEDO_HANDLE *) malloc(sizeof(WEDO_HANDLE));
    pWedo->pHub=NULL;
    pWedo->pHubDevice=NULL;
    pWedo->powerA=0;
    pWedo->powerB=0;
    pWedo->A_connection_type=NONE_CONNECT;
    pWedo->B_connection_type=NONE_CONNECT;
    pWedo->hPollThread=NULL;
    pWedo->tilt=0;
    pWedo->distance=1000;
    pWedo->pTransfer=NULL;
}

static void __wedo_request_exit(int code)
{
    do_exit = code;
}


DWORD WINAPI __wedo_poll_thread_func( LPVOID lpParam ){
    int r = 0;
    //printf("poll thread running\n");

    while (do_exit==0) {
        struct timeval tv = { 0,500000 };
        r = libusb_handle_events_timeout_completed(NULL, &tv,NULL);
        if (r < 0) {
            __wedo_request_exit(2);
            break;
        }
    }

    //printf("poll thread shutting down\n");
    return 0;
}

static int __wedo_parse_data(const int ids[],const int datas[],const int idList[]) {
    int i, j;
    for (i = 0; i < 2; i++) {
        for (j = 0; idList[j] >= 0; j++) {
            if (ids[i] == idList[j]) {
                return datas[i];
            }
        }
    }

    return -1;
}


static void __wedo_get_tilt(const int ids[],const int datas[]) {
    int code;
    code = __wedo_parse_data(ids,datas,TILT_SENSORS);
    if (code < 0) {
        return;
    }
    if (10 <= code && code <= 40) {
        pWedo->tilt=WEDO_TILT_BACK;
    } else if (60 <= code && code <= 90) {
        pWedo->tilt=WEDO_TILT_RIGHT;
    } else if (170 <= code && code <= 190) {
        pWedo->tilt=WEDO_TILT_FORWARD;
    } else if (220 <= code && code <= 240) {
        pWedo->tilt=WEDO_TILT_LEFT;
    } else {
        pWedo->tilt = WEDO_TILT_NONE;
    }
};

static void __wedo_get_distance(const int ids[],const int datas[]) {
    int code;
    code = __wedo_parse_data(ids,datas,DISTANCE_SENSORS);
    if (code < 0) {
        return;
    }
    pWedo->distance = (code<68)?0:(code - 68);
}




static void LIBUSB_CALL __wedo_transfer_cb(struct libusb_transfer *transfer)
{
    if (transfer->status==LIBUSB_TRANSFER_COMPLETED) {
        int ids[2];
        int datas[2];
        int i=0;
        do {
            /*
 sensorData = {rawData[3]: rawData[2], rawData[5]: rawData[4]}
 */
            ids[0]=transfer->buffer[i+3];
            datas[0]=transfer->buffer[i+2];
            ids[1]=transfer->buffer[i+5];
            datas[1]=transfer->buffer[i+4];
            __wedo_get_tilt(ids,datas);
            __wedo_get_distance(ids,datas);
            //printf("%d:%d %d:%d\n",ids[0],datas[0],ids[1],datas[1]);
            i+=9;
        } while(i<transfer->actual_length);
    }


    if (libusb_submit_transfer(pWedo->pTransfer) < 0)
        __wedo_request_exit(2);

}

int wedo_init() {
    int rc;
    DWORD  dwPollThread;
    if (pWedo != NULL) {
        return -1;
    }
    __wedo_init_data();
    rc=libusb_init(NULL);
    if (rc < 0) {
        return rc;
    }
    pWedo->pHub=libusb_open_device_with_vid_pid(NULL,ID_VENDOR,ID_PRODUCT);
    if (pWedo->pHub == NULL) {
        return -1;
    }
    pWedo->pHubDevice=libusb_get_device(pWedo->pHub);
    libusb_set_configuration(pWedo->pHub, WEDO_CONFIGURATION);
    __wedo_check_powers();
    pWedo->hPollThread=CreateThread(NULL,0,__wedo_poll_thread_func,NULL,0,&dwPollThread);
    pWedo->pTransfer=libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(pWedo->pTransfer,pWedo->pHub,0x81,pWedo->buffer,
                              sizeof(pWedo->buffer),__wedo_transfer_cb,NULL,0);
    libusb_submit_transfer(pWedo->pTransfer);
    return 0;
}

static boolean __wedo_in_list(const int id, const int idList[]) {
    for (int i=0;idList[i]!=-1;i++) {
        if (id==idList[i]){
            return true;
        }
    }
    return false;
}
static enum connect_type __wedo_check_connection_type(int id) {
    if (__wedo_in_list(id, TILT_SENSORS)){
        return TILT_SENSOR;
    }
    if (__wedo_in_list(id, DISTANCE_SENSORS)){
        return DISTANCE_SENSOR;
    }
    if (__wedo_in_list(id, MOTORS)){
        return MOTOR;
    }
    if (__wedo_in_list(id, LIGHTS)){
        return LIGHT;
    }
    if (__wedo_in_list(id,SERVO_MOTORS)){
        return SERVO_MORTOR;
    }
    return NONE_CONNECT;
}
static void __wedo_check_powers() {
    unsigned char recv_data[8];
    int recv_len;
    int rc;
    rc=libusb_bulk_transfer(pWedo->pHub,0x81,recv_data,sizeof(recv_data),&recv_len,1000);
    if (rc<0) {
        printf("check failed!");
        return;
    }
    printf("test:%d %d\n",recv_data[3],recv_data[5]);
    pWedo->A_connection_type=__wedo_check_connection_type(recv_data[3]);
    pWedo->B_connection_type=__wedo_check_connection_type(recv_data[5]);
    //printf("%d %d\n",pWedo->A_connection_type,pWedo->B_connection_type);


}

void wedo_close() {
    if (pWedo == NULL) {
        return;
    }
    __wedo_request_exit(1);
    wedo_stop_mortor();
    wedo_light_off();
    libusb_close(pWedo->pHub);
    free(pWedo);
    pWedo = NULL;
    libusb_exit(NULL);
}

/**
 * 将电机动力值调整到[-100,100]区间
 * @param value
 * @return
 */
static int __wedo_normalize_motor_power(int value) {
    if (value > 100) {
        value = 100;
    } else if (value < -100) {
        value = -100;
    }
    if (value > 0) {
        value += 27;
    } else if (value < 0) {
        value -= 27;
    }
    return value;
}

static int __wedo_set_power(int powerA, int powerB) {
    unsigned char send_data[8];
    int send_len;
    if (pWedo == NULL || pWedo->pHub == NULL) {
        return 0;
    }
    memset(send_data, 0, sizeof(send_data));
    libusb_claim_interface(pWedo->pHub, 0);
    pWedo->powerA=powerA;
    pWedo->powerB=powerB;
    send_data[0] = 0x40;
    send_data[1] = powerA & 0xFF;
    send_data[2] = powerB & 0xFF;
    //printf("%d %d\n",send_data[1],send_data[2]);
    send_len = libusb_control_transfer(pWedo->pHub,
                                       0x21 ,
                                       0x09 ,
                                       0x0200 ,
                                       0, send_data, sizeof(send_data), 0);
    if (send_len < sizeof(send_data)) {
        return -1;
    }
    libusb_release_interface(pWedo->pHub, 0);
    return 0;
}

static int __wedo_set_motor(int valueA, int valueB) {
    if (pWedo==NULL) {
        return -1;
    }
    int powerA = (pWedo->A_connection_type==MOTOR)?__wedo_normalize_motor_power(valueA):pWedo->powerA;
    int powerB = (pWedo->B_connection_type==MOTOR)?__wedo_normalize_motor_power(valueB):pWedo->powerB;
    return __wedo_set_power(powerA,powerB);
}

int wedo_start_mortor(int power) {
    return __wedo_set_motor(power, power);
}

int wedo_stop_mortor() {
    return __wedo_set_motor(0, 0);
};

int wedo_start_mortor_a(int power) {
    return __wedo_set_motor(power, pWedo->powerB);
};

int wedo_stop_mortor_a() {
    return __wedo_set_motor(0, pWedo->powerB);
};

int wedo_start_mortor_b(int power) {
    return __wedo_set_motor(pWedo->powerA, power);
};

int wedo_stop_mortor_b() {
    return __wedo_set_motor(pWedo->powerA, 0);
};

int wedo_get_tilt(){
    if (pWedo==NULL) {
        return -1;
    }
    return pWedo->tilt;
}
int wedo_get_distance(){
    if (pWedo==NULL) {
        return -1;
    }
    return pWedo->distance;
}
static int __wedo_set_light(int valueA,int valueB) {
    if (pWedo==NULL) {
        return -1;
    }
    int powerA = (pWedo->A_connection_type==LIGHT)?__wedo_normalize_motor_power(valueA):pWedo->powerA;
    int powerB = (pWedo->B_connection_type==LIGHT)?__wedo_normalize_motor_power(valueB):pWedo->powerB;
    return __wedo_set_power(powerA,powerB);
}
int wedo_light_on() {
    return __wedo_set_light(100,100);
}
int wedo_light_off() {
    return __wedo_set_light(0,0);
}
int wedo_light_on_a() {
    return __wedo_set_light(100,pWedo->powerB);
}
int wedo_light_off_a() {
    return __wedo_set_light(0,pWedo->powerB);
}
int wedo_light_on_b() {
    return __wedo_set_light(pWedo->powerA,100);
}
int wedo_light_off_b() {
    return __wedo_set_light(pWedo->powerA,0);
}

static int __wedo_normalize_degree(int degree){
    degree=degree*127/90;
    if (degree>127) {
        degree=127;
    }else if (degree<-127){
        degree=-127;
    }
    return degree;
}
static int __wedo_servo_set(int degreeA,int degreeB){
    if (pWedo==NULL) {
        return -1;
    }
    int powerA =(pWedo->A_connection_type==SERVO_MORTOR)?__wedo_normalize_degree(degreeA):pWedo->powerA;
    int powerB =(pWedo->B_connection_type==SERVO_MORTOR)?__wedo_normalize_degree(degreeB):pWedo->powerB;
    return __wedo_set_power(powerA,powerB);
}
int wedo_servo_rotate(int degree){
    return __wedo_servo_set(degree,degree);
}
int wedo_servo_off(){
    return __wedo_servo_set(0,0);
}


