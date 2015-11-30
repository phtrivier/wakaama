/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Simon Bernard
 *******************************************************************************/
#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Resource Id's:
#define RES_COLOUR          5706
#define RES_ON_OFF          5850

#define BLUE_COLOR          "#0000FF"

const char *WLAN_LED_TRIGGER = "/sys/devices/platform/leds-gpio/leds/wlan/trigger";

void write_trigger_file(char *value) {
    FILE *fileStream; 
    char trigger [100];
    fileStream = fopen (WLAN_LED_TRIGGER, "w"); 
    fprintf(fileStream, "%s", value);
    fclose(fileStream);
}

void switchon() {
    fprintf(stdout, "\n\t LIGHT ON\r\n\n");
    write_trigger_file("timer");
}

void switchoff() {
    fprintf(stdout, "\n\t LIGHT OFF\r\n\n");
    write_trigger_file("none");
}

bool is_led_on() {
    FILE *fileStream; 
    char trigger [100];
    fileStream = fopen (WLAN_LED_TRIGGER, "r"); 
    fgets (trigger, 100, fileStream); 
    bool on = (strstr(trigger, "[timer]") != NULL);

    fclose(fileStream);
    return on;
}

static uint8_t prv_set_value(lwm2m_data_t * dataP) {
    // a simple switch structure is used to respond at the specified resource asked
    switch (dataP->id) {
    case RES_COLOUR: {

        dataP->value  = (uint8_t*)BLUE_COLOR;
        dataP->length = strlen(BLUE_COLOR);
        dataP->flags  = LWM2M_TLV_FLAG_STATIC_DATA;
        dataP->type     = LWM2M_TYPE_RESOURCE;
        dataP->dataType = LWM2M_TYPE_STRING;
        return COAP_205_CONTENT ;
    }
    case RES_ON_OFF: {
        bool on = is_led_on();
        lwm2m_data_encode_bool(on, dataP);
        dataP->type = LWM2M_TYPE_RESOURCE;
        return COAP_205_CONTENT ;
    }
    default:
        return COAP_404_NOT_FOUND ;
    }
}

static uint8_t prv_led_read(uint16_t instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP) {
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND ;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0) {

        uint16_t resList[] = {
        RES_COLOUR,
        RES_ON_OFF };
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR ;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do {
        result = prv_set_value((*dataArrayP) + i);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT );

    return result;
}

static uint8_t prv_led_write(uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP) {
    int i;
    uint8_t result;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND ;
    }

    i = 0;

    do {
        switch (dataArray[i].id) {
        case RES_ON_OFF: ;
            bool on;
            if (1 == lwm2m_data_decode_bool(dataArray + i, &on)) {
                if (on) {
                    switchon();
                } else {
                    switchoff();
                }
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
            break;
        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED );

    return result;
}

static void prv_led_close(lwm2m_object_t * objectP) {
    LWM2M_LIST_FREE(objectP->instanceList);
}

void display_led_object(lwm2m_object_t * object) {
#ifdef WITH_LOGS
    fprintf(stdout, "  /%u: led object:\r\n", object->objID);
#endif
}

lwm2m_object_t * get_object_led() {

    /*
     * The get_object_led function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * ledObj;

    ledObj = (lwm2m_object_t *) lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != ledObj) {
        memset(ledObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3313 is the standard ID for the mandatory object "IPSO Light Control".
         */
        ledObj->objID = LWM2M_LIGHT_OBJECT_ID;

        /*
         * there is only one instance of LED on the Application Board for mbed NXP LPC1768.
         *
         */
        ledObj->instanceList = (lwm2m_list_t *) lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != ledObj->instanceList) {
            memset(ledObj->instanceList, 0, sizeof(lwm2m_list_t));
        } else {
            lwm2m_free(ledObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        ledObj->readFunc = prv_led_read;
        ledObj->writeFunc = prv_led_write;
        ledObj->executeFunc = NULL;
        ledObj->closeFunc = prv_led_close;
    }

    return ledObj;
}