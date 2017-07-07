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

const int TILTSENSOR[] = {38, 39, -1};
const int DISTANCESENSOR[] = {176, 177, 178, 179, -1};
const int MOTORS[] = {238,231,237,239, -1};
const int LIGHTS[] = {203,-1};

typedef struct {
    struct libusb_device_handle *pHub;
    struct libusb_device *pHubDevice;
    int powerA;
    int powerB;
    boolean motor_A_connected;
    boolean motor_B_connected;
    volatile int distance;
    volatile int tilt;
    HANDLE hPollThread;
    struct libusb_transfer* pTransfer;
    unsigned char buffer[100];
} WEDO_HANDLE;
static WEDO_HANDLE *pWedo = NULL;
static volatile int do_exit=0;

static void __wedo_check_motors();

static void __wedo_init_data() {
    pWedo = (WEDO_HANDLE *) malloc(sizeof(WEDO_HANDLE));
    pWedo->pHub=NULL;
    pWedo->pHubDevice=NULL;
    pWedo->powerA=0;
    pWedo->powerB=0;
    pWedo->motor_A_connected=false;
    pWedo->motor_B_connected=false;
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
    code = __wedo_parse_data(ids,datas,TILTSENSOR);
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
    }
    pWedo->tilt=WEDO_TILT_NONE;
};

static void __wedo_get_distance(const int ids[],const int datas[]) {
    int code;
    code = __wedo_parse_data(ids,datas,DISTANCESENSOR);
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
    __wedo_check_motors();
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

static void __wedo_check_motors() {
    unsigned char recv_data[8];
    int recv_len;
    int rc;
    rc=libusb_bulk_transfer(pWedo->pHub,0x81,recv_data,sizeof(recv_data),&recv_len,1000);
    if (rc<0) {
        printf("check failed!");
        return;
    }
    //printf("test:%d %d\n",recv_data[3],recv_data[5]);
    if (__wedo_in_list(recv_data[3],MOTORS)) {
        pWedo->motor_A_connected=true;
    }
    if (__wedo_in_list(recv_data[5],MOTORS)){
        pWedo->motor_B_connected=true;
    }

}

void wedo_close() {
    if (pWedo == NULL) {
        return;
    }
    __wedo_request_exit(1);
    wedo_stop_mortor();
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

static int __wedo_start_mortor(int valueA, int valueB) {
    unsigned char send_data[8];
    int send_len;
    if (pWedo == NULL || pWedo->pHub == NULL) {
        return 0;
    }
    memset(send_data, 0, sizeof(send_data));
    pWedo->powerA = __wedo_normalize_motor_power(valueA);
    pWedo->powerB = __wedo_normalize_motor_power(valueB);
    libusb_claim_interface(pWedo->pHub, 0);
    send_data[0] = 0x40;
    send_data[1] = pWedo->motor_A_connected ? pWedo->powerA & 0xFF : 0;
    send_data[2] = pWedo->motor_B_connected ? pWedo->powerB & 0xFF : 0;
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

int wedo_start_mortor(int power) {
    return __wedo_start_mortor(power, power);
}

int wedo_stop_mortor() {
    return __wedo_start_mortor(0, 0);
};

int wedo_start_mortor_a(int power) {
    return __wedo_start_mortor(power, pWedo->powerB);
};

int wedo_stop_mortor_a() {
    return __wedo_start_mortor(0, pWedo->powerB);
};

int wedo_start_mortor_b(int power) {
    return __wedo_start_mortor(pWedo->powerA, power);
};

int wedo_stop_mortor_b() {
    return __wedo_start_mortor(pWedo->powerA, 0);
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


