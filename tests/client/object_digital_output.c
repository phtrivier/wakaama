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
#define RES_STATE          5550

// File Descriptor for the value and
const char *GPIO_EXPORT = "/sys/class/gpio/export";
const char *GPIO_D13_VALUE = "/sys/class/gpio/D13/value";
const char *GPIO_D13_DIRECTION = "/sys/class/gpio/D13/direction";

void write_gpio_d13_value(char *value) {
  FILE *fileStream; 
  fileStream = fopen (GPIO_D13_VALUE, "w"); 
  fprintf(fileStream, "%s", value);
  fclose(fileStream);
}

void write_gpio_d13_direction(char *value) {
  FILE *fileStream; 
  fileStream = fopen (GPIO_D13_DIRECTION, "w");
  fprintf(fileStream, "%s", value);
  fclose(fileStream);
}

void export_gpio_d13() {
  FILE *fileStream; 
  fileStream = fopen (GPIO_EXPORT, "w");
  fprintf(fileStream, "%s", "115");
  fclose(fileStream);
}

void prepare_gpio_d13() {
  fprintf(stdout, "\n\t EXPORTING GPIO 115\n");
  export_gpio_d13();
  fprintf(stdout, "\n\t SETTING LED DIRECTION OUT\n");
  write_gpio_d13_direction("out");
}
  
void prv_switch_digital_output_on() {
  fprintf(stdout, "\n\t OUTPUT ON\r\n\n");
  write_gpio_d13_value("1");
}

void prv_switch_digital_output_off() {
  fprintf(stdout, "\n\t OUTPUT OFF\r\n\n");
  write_gpio_d13_value("0");
}

bool is_gpio_d13_on() {
  FILE *fileStream; 
  char trigger [8];
  fileStream = fopen (GPIO_D13_VALUE, "r"); 
  fgets (trigger, 8, fileStream); 
  bool on = (strstr(trigger, "1") != NULL);
  fclose(fileStream);
  return on;
}

static uint8_t prv_set_value(lwm2m_data_t * dataP) {
  // a simple switch structure is used to respond at the specified resource asked
  switch (dataP->id) {
  case RES_STATE: {
    bool on = is_gpio_d13_on();
    lwm2m_data_encode_bool(on, dataP);
    dataP->type = LWM2M_TYPE_RESOURCE;
    return COAP_205_CONTENT ;
  }
  default:
    return COAP_404_NOT_FOUND ;
  }
}

static uint8_t prv_digital_output_read(uint16_t instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP) {
  uint8_t result;
  int i;

  // this is a single instance object
  if (instanceId != 0) {
    return COAP_404_NOT_FOUND ;
  }

  // is the server asking for the full object ?
  if (*numDataP == 0) {

    uint16_t resList[] = {
      RES_STATE
    };
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

static uint8_t prv_digital_output_write(uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP) {
  int i;
  uint8_t result;

  // this is a single instance object
  if (instanceId != 0) {
    return COAP_404_NOT_FOUND ;
  }

  i = 0;

  do {
    switch (dataArray[i].id) {
    case RES_STATE: ;
      bool on;
      if (1 == lwm2m_data_decode_bool(dataArray + i, &on)) {
        if (on) {
          prv_switch_digital_output_on();
        } else {
          prv_switch_digital_output_off();
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

static void prv_digital_output_close(lwm2m_object_t * objectP) {
  LWM2M_LIST_FREE(objectP->instanceList);
}

void display_digital_output_object(lwm2m_object_t * object) {
#ifdef WITH_LOGS
  fprintf(stdout, "  /%u: digital_output object:\r\n", object->objID);
#endif
}


lwm2m_object_t * get_digital_output_object() {
  
    /*
     * The get_object_digital_ouput function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * ledObj;

    ledObj = (lwm2m_object_t *) lwm2m_malloc(sizeof(lwm2m_object_t));

    /*
     * Use the LED as an output
     */
    prepare_gpio_d13();
    
    if (NULL != ledObj) {
        memset(ledObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3201 is the standard ID for the mandatory object "IPSO Digital Output".
         */
        ledObj->objID = LWM2M_DIGITAL_OUTPUT_OBJECT_ID;

        /*
         * There is only one instance of LED on the Application Board for Yun.
         * We use it as a digital output
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
        ledObj->readFunc = prv_digital_output_read;
        ledObj->writeFunc = prv_digital_output_write;
        ledObj->executeFunc = NULL;
        ledObj->closeFunc = prv_digital_output_close;
    }

    return ledObj;
}
