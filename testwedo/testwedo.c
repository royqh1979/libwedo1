//
// Created by Roy on 2017/7/7.
//
#include <libusb-1.0/libusb.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define ID_VENDOR 0x0694
#define ID_PRODUCT 0x0003
#define WEDO_CONFIGURATION 1

static volatile int do_exit=0;
static HANDLE hPollThread;
static struct libusb_transfer *pWedoTransfer;
static unsigned char buffer[100];
static struct libusb_device_handle* pHub;
static struct libusb_device* pDevice;

static void request_exit(int code)
{
    do_exit = code;
}

DWORD WINAPI poll_thread_func( LPVOID lpParam ){
    int r = 0;
    printf("poll thread running\n");

    while (do_exit==0) {
        struct timeval tv = { 1, 0 };
        r = libusb_handle_events_timeout(NULL, &tv);
        if (r < 0) {
            request_exit(2);
            break;
        }
    }

    printf("poll thread shutting down\n");
    return 0;
}


static void LIBUSB_CALL wedo_transfer_cb(struct libusb_transfer *transfer)
{

    printf("recv:%d\n",transfer->status);
    for (int i=0;i<transfer->actual_length;i++) {
        printf("%d ",transfer->buffer[i]);
        if ((i+1)%8==0) {
            printf("\n");
        }
    }

    if (libusb_submit_transfer(pWedoTransfer) < 0)
        request_exit(2);
}


int main() {
    int rc;
    DWORD dwPollThread;
    rc=libusb_init(NULL);
    if (rc<0) {
        printf("init error:%d\n",rc);
        return 0;
    }
    pHub=libusb_open_device_with_vid_pid(NULL,ID_VENDOR,ID_PRODUCT);
    if (pHub==NULL) {
        printf("open device error\n");
        return 0;
    }
    pDevice=libusb_get_device(pHub);
    libusb_set_configuration(pHub,WEDO_CONFIGURATION);

    hPollThread=CreateThread(NULL,0,poll_thread_func,NULL,0,&dwPollThread);
    pWedoTransfer=libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(pWedoTransfer,pHub,0x81,buffer,
        sizeof(buffer),wedo_transfer_cb,NULL,0);
    libusb_submit_transfer(pWedoTransfer);

    while(1) {
        Sleep(10);
    }

}

