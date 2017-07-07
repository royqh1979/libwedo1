//
// Created by Roy on 2017/7/7.
//
#include <stdio.h>
#include <windows.h>
#include "wedo1.h"

int main() {
    int rc;
    rc=wedo_init();
    printf("init:%d\n",rc);
    //printf("dis:%d\n",wedo_get_distance());
    rc=wedo_start_mortor(100);
    printf("motor:%d\n",rc);
    //Sleep(5000);
    //rc=wedo_stop_mortor();
    //printf("init:%d\n",rc);
    for (int i=0;i<10;i++) {
        //printf("dis:%d\n",wedo_get_distance());
        //printf("tilt:%d\n",wedo_get_tilt());
        wedo_start_mortor(wedo_get_distance());
        Sleep(1000);
    }
    wedo_stop_mortor();
    wedo_close();
    return 0;
}

