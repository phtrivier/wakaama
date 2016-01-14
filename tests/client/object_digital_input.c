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
#define RES_STATE          5500

// File Descriptor for the value and
const char *GPIO_D2_EXPORT = "/sys/class/gpio/export";
const char *GPIO_D2_VALUE = "/sys/class/gpio/D2/value";
const char *GPIO_D2_DIRECTION = "/sys/class/gpio/D2/direction";

void write_gpio_d2_value(char *value) {
  FILE *fileStream; 
  fileStream = fopen (GPIO_D2_VALUE, "w"); 
  fprintf(fileStream, "%s", value);
  fclose(fileStream);
}

void write_gpio_d2_direction(char *value) {
  FILE *fileStream; 
  fileStream = fopen (GPIO_D2_DIRECTION, "w");
  fprintf(fileStream, "%s", value);
  fclose(fileStream);
}

void export_gpio_d2() {
  FILE *fileStream; 
  fileStream = fopen (GPIO_D2_EXPORT, "w");
  fprintf(fileStream, "%s", "117");
  fclose(fileStream);
}

void prepare_gpio_d2() {
  fprintf(stdout, "\n\t EXPORTING GPIO117\n");
  export_gpio_d2();
  fprintf(stdout, "\n\t SETTING D2 DIRECTION IN\n");
  write_gpio_d2_direction("in");
}

bool is_gpio_d2_on() {
  FILE *fileStream; 
  char trigger [8];
  fileStream = fopen (GPIO_D2_VALUE, "r"); 
  fgets (trigger, 8, fileStream); 
  bool on = (strstr(trigger, "1") != NULL);
  fclose(fileStream);
  return on;
}

static uint8_t prv_set_value(lwm2m_data_t * dataP) {

  fprintf(stdout, "!!! Reading id %d\n", dataP->id);
  
  // a simple switch structure is used to respond at the specified resource asked
  switch (dataP->id) {
  case RES_STATE: {
    bool on = is_gpio_d2_on();
    lwm2m_data_encode_bool(on, dataP);
    dataP->type = LWM2M_TYPE_RESOURCE;
    return COAP_205_CONTENT ;
  }
  case 5502 : {
    lwm2m_data_encode_bool(true, dataP);
    dataP->type = LWM2M_TYPE_RESOURCE;
    return COAP_205_CONTENT ;
  }
  default:
    lwm2m_data_encode_int(42, dataP);
    dataP->type = LWM2M_TYPE_RESOURCE;
    return COAP_205_CONTENT ;

    // Just a test to return everything
    // return COAP_404_NOT_FOUND ;
  }
}

static uint8_t prv_digital_input_read(uint16_t instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP) {
  uint8_t result;
  int i;

  
  
  // this is a single instance object
  if (instanceId != 0) {
    return COAP_404_NOT_FOUND ;
  }

  // is the server asking for the full object ?
  if (*numDataP == 0) {

    uint16_t resList[] = {
      RES_STATE,
      5501, 5502, 5503, 5504, 5750,5751
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

static uint8_t prv_digital_input_write(uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP) {
  int i;
  uint8_t result;

  // this is a single instance object
  if (instanceId != 0) {
    return COAP_404_NOT_FOUND ;
  }

  i = 0;

  do {
    switch (dataArray[i].id) {
    default:
      result = COAP_405_METHOD_NOT_ALLOWED;
    }

    i++;
  } while (i < numData && result == COAP_204_CHANGED );

  return result;
}

static void prv_digital_input_close(lwm2m_object_t * objectP) {
  LWM2M_LIST_FREE(objectP->instanceList);
}

static void prv_digital_input_gpio(uint16_t gpioId, char * bytes, uint16_t bytesCount, lwm2m_object_t * objectP, lwm2m_context_t * contextP) {
  static char dgi = '0';

  if (gpioId == 117 && bytesCount == 2) {
    char value = bytes[0];
    if ((value == '1' && dgi == '0') || (value == '0' && dgi == '1') ){

      printf("Value changed, notifying\n");

      lwm2m_uri_t uri;
      lwm2m_stringToUri("/3200/0/5500",
                        14,
                        &uri);

      // This assumes the context as been set as user data 
      // lwm2m_context_t * lwm2mH = (lwm2m_context_t *) objectP->userData;

      lwm2m_context_t * lwm2mH = contextP;
      lwm2m_resource_value_changed(lwm2mH, &uri);
      dgi = value;
    }    
  }
}

void display_digital_input_object(lwm2m_object_t * object) {
#ifdef WITH_LOGS
  fprintf(stdout, "  /%u: digital_input object:\r\n", object->objID);
#endif
}


lwm2m_object_t * get_digital_input_object() {
  
    /*
     * The get_object_digital_ouput function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * digital_input_obj;

    digital_input_obj = (lwm2m_object_t *) lwm2m_malloc(sizeof(lwm2m_object_t));

  
    /*
     * Use the LED as an ouput
     */
    if (NULL != digital_input_obj) {

      prepare_gpio_d2();
          
        memset(digital_input_obj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3201 is the standard ID for the mandatory object "IPSO Digital Output".
         */
        digital_input_obj->objID = LWM2M_DIGITAL_INPUT_OBJECT_ID;

        /*
         * There is only one instance of LED on the Application Board for Yun.
         * We use it as a digital output
         *
         */
        digital_input_obj->instanceList = (lwm2m_list_t *) lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != digital_input_obj->instanceList) {
            memset(digital_input_obj->instanceList, 0, sizeof(lwm2m_list_t));
        } else {
            lwm2m_free(digital_input_obj);
            return NULL;
        }
        
        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        digital_input_obj->readFunc = prv_digital_input_read;
        digital_input_obj->writeFunc = prv_digital_input_write;
        digital_input_obj->executeFunc = NULL;
        digital_input_obj->closeFunc = prv_digital_input_close;

        digital_input_obj->gpioFunc = prv_digital_input_gpio;
        
    }

    return digital_input_obj;
}
