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
#define RES_VALUE          5700

// File Descriptor for the value and
const char *ANALOG_A0_ENABLE = "/sys/bus/iio/devices/iio:device0/enable";
const char *ANALOG_A0_VALUE = "/sys/bus/iio/devices/iio:device0/in_voltage_A0_raw";

void write_analog_a0_value(char *value) {
  FILE *fileStream; 
  fileStream = fopen (ANALOG_A0_VALUE, "w"); 
  fprintf(fileStream, "%s", value);
  fclose(fileStream);
}

void prepare_analog_a0() {
  FILE *fileStream; 
  fileStream = fopen (ANALOG_A0_ENABLE, "w");
  fprintf(fileStream, "%s", "1");
  fclose(fileStream);
}

uint16_t get_analog_a0_value() {
  FILE *fileStream; 
  char content [8];
  fileStream = fopen (ANALOG_A0_VALUE, "r"); 
  fgets (content, 8, fileStream);

  printf("[get_analog_0_value] File content ? %s\n", content);
  
  uint16_t temperature = atoi(content);

  printf("[get_analog_0_value] Value ? %d\n", temperature);
  
  fclose(fileStream);
  return temperature;
}

static uint8_t prv_set_value(lwm2m_data_t * dataP) {
  // a simple switch structure is used to respond at the specified resource asked
  switch (dataP->id) {
  case RES_VALUE: {
    // FIXME(pht) encode float from string ? 
    uint16_t temperature = get_analog_a0_value();
    lwm2m_data_encode_float(temperature, dataP);
    dataP->type = LWM2M_TYPE_RESOURCE;
    return COAP_205_CONTENT ;
  }
  default:
    return COAP_404_NOT_FOUND ;
  }
}

static uint8_t prv_temperature_sensor_read(uint16_t instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP) {
  uint8_t result;
  int i;

  // this is a single instance object
  if (instanceId != 0) {
    return COAP_404_NOT_FOUND ;
  }

  // is the server asking for the full object ?
  if (*numDataP == 0) {

    uint16_t resList[] = {
      RES_VALUE
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

static uint8_t prv_temperature_sensor_write(uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP) {
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

static void prv_temperature_sensor_close(lwm2m_object_t * objectP) {
  LWM2M_LIST_FREE(objectP->instanceList);
}

static void prv_temperature_sensor_analog(uint16_t ioId, char * bytes, uint16_t bytesCount, lwm2m_object_t * objectP, struct lwm2m_context_t * contextP) {

   //printf("In Temperature callback\n");
  
  // TODO(pht) store as float ? 
  static uint16_t last_temperature = 0;

  if (ioId == 0 && bytesCount > 0 ) {

    char buffer[20];
    strncpy(buffer, bytes, bytesCount);

    // printf("Bytes read ? %s\n", buffer);
    
    uint16_t temperature_value = atoi(buffer);
    
    // printf("Temperature read ? %d\n", temperature_value);
    // printf("Last Temperature read ? %d\n", last_temperature);
    
    if (temperature_value != last_temperature && abs(temperature_value - last_temperature) > 3) {
      printf("Value changed, to %d, notifying\n", temperature_value);

      lwm2m_uri_t uri;
      lwm2m_stringToUri("/3303/0/5700",
                        14,
                        &uri);

      lwm2m_context_t * lwm2mH = contextP;
      lwm2m_resource_value_changed(lwm2mH, &uri);
      
      last_temperature = temperature_value;
    }
      
  }
}

void display_temperature_sensor_object(lwm2m_object_t * object) {
#ifdef WITH_LOGS
  fprintf(stdout, "  /%u: temperature_sensor object:\r\n", object->objID);
#endif
}


lwm2m_object_t * get_temperature_sensor_object() {
  
    /*
     * The get_object_digital_ouput function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * temperature_sensor_obj;

    temperature_sensor_obj = (lwm2m_object_t *) lwm2m_malloc(sizeof(lwm2m_object_t));

  
    /*
     * Use the LED as an ouput
     */
    if (NULL != temperature_sensor_obj) {

      prepare_analog_a0();
          
        memset(temperature_sensor_obj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3201 is the standard ID for the mandatory object "IPSO Digital Output".
         */
        temperature_sensor_obj->objID = LWM2M_TEMPERATURE_SENSOR_OBJECT_ID;

        /*
         * There is only one instance of LED on the Application Board for Yun.
         * We use it as a digital output
         *
         */
        temperature_sensor_obj->instanceList = (lwm2m_list_t *) lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != temperature_sensor_obj->instanceList) {
            memset(temperature_sensor_obj->instanceList, 0, sizeof(lwm2m_list_t));
        } else {
            lwm2m_free(temperature_sensor_obj);
            return NULL;
        }
        
        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        temperature_sensor_obj->readFunc = prv_temperature_sensor_read;
        temperature_sensor_obj->writeFunc = prv_temperature_sensor_write;
        temperature_sensor_obj->executeFunc = NULL;
        temperature_sensor_obj->closeFunc = prv_temperature_sensor_close;
        temperature_sensor_obj->analogFunc = prv_temperature_sensor_analog;
        
    }

    return temperature_sensor_obj;
}
