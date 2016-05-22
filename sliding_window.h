/*
FILE:assignment.h
AUTHOR:Jordan Pinglin Chou
USERNAME:18348691
UNIT: Computer Communications
PURPOSE: Header file for assignment.c
REFERENCE:-
COMMENTS:-
REQUIRES:-
*/
#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H
#include <cnet.h>
#include <stdlib.h>
#include <stdio.h>

#define BETWEEN(a,b,c) (((a<=b) && (b< c)) || ((c< a) && (a<=b)) || ((b< c) && (c< a)))
#define FRAME_SIZE(f) ((sizeof(Frame) - sizeof(f.data)) + f.length)
#define WINDOW_SIZE 3

typedef enum
{
    DATA, ACK
} KIND;

typedef struct
{
    int source;
    int dest;
    int data[MAX_MESSAGE_SIZE];
} Packet;


typedef struct
{
    int source;
    int dest;
    KIND kind;
    size_t length;
    int checksum;
    int sequence;
    char data[MAX_MESSAGE_SIZE];
} Frame;

void application_down_to_network(CnetEvent ev, CnetTimerID, CnetData data);
void physical_up_to_datalink(CnetEvent ev, CnetTimerID, CnetData data);
void network_up_to_application(Frame frame, int link);
void network_down_to_datalink(Frame frame);
void datalink_up_to_network(Frame frame, int link, int length);
void datalink_down_to_physical_transmit(Frame frame, int link);
void physical_up_to_datalink(CnetEvent ev, CnetTimerID, CnetData data);
void set_timer(Frame frame, int link, int sequence);
void datalink_down_to_physical_ack(int dest, int link, int sequence);
void increment(int* num);
void datalink_down_to_physical_forward(Frame frame, int link);
void datalink_up_to_network_ack(Frame frame, int link);
void timeout_link_1(CnetEvent ev, CnetTimerID timer, CnetData data);
void timeout_link_2(CnetEvent ev, CnetTimerID timer, CnetData data);
void timeout_link_3(CnetEvent ev, CnetTimerID timer, CnetData data);
void timeout_link_4(CnetEvent ev, CnetTimerID timer, CnetData data);
void print_window(int link);

//Array to store the timers, one for each link, max of 4 links
static CnetTimerID timers[4][WINDOW_SIZE+1];

//Contains the names of each node
static char* nodes[7] = {"Australia", "Fiji","New Zealand","Indonesia","Singapore","Malaysia","Brunei"};

//The number of messages in the window. One for each link
static int num_in_window[4] = {0,0,0,0};

//The number of messages in store-forward buffer, one for each link
static int num_in_buffer[4] = {0,0,0,0};

//The frames stored in each window
static Frame window[4][WINDOW_SIZE + 1];

//The bounds of the store-forward buffer
static int buffer_bounds[4][3] = {{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}};

//The frames stored in the store-forward buffer
static Frame buffer[4][WINDOW_SIZE+1];

//The frame expected for each link
static int frame_expected[4] = {0,0,0,0};

//The ack expected from each link
static int ack_expected[4] = {0,0,0,0};

//The next frame expected from each link
static int next_frame[4] = {0,0,0,0};

static int routing_table[7][7] =
      {{0,1,2,3,3,3,3},   //Australia
       {1,0,1,1,1,1,1},   //Fiji
       {1,1,0,1,1,1,1},   //NewZealand
       {1,1,1,0,2,3,4},   //Indonesia
       {1,1,1,1,0,2,1},   //Singapore
       {1,1,1,1,2,0,1},   //Malaysia
       {1,1,1,1,1,1,0}};  //BruneiB

#endif
