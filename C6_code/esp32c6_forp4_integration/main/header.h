/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef ESPNOW_EXAMPLE_H
#define ESPNOW_EXAMPLE_H
// WebSocket server
#define WS_SERVER_URI  "ws://192.168.29.230:3031"

// UART send to P4 — defined in uart file, used in csix.c
void uart_data_send(const char *msg);

// Socket init
void socket_init_c6(void);
#endif
