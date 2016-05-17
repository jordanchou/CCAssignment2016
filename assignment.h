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

#include <cnet.h>
#include <stdlib.h>
#include <stdio.h>

#define BETWEEN(a,b,c) (((a<=b) && (b< c)) || ((c< a) && (a<=b)) || ((b< c) && (c< a)))
#define FRAME_SIZE(f) ((sizeof(Frame) - sizeof(f.data)) + f.length)
#define WINDOW_SIZE 5

typedef enum
{
    DATA, ACK
} KIND;

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
void datalink_down_to_physical_ack(int link, int sequence);
void increment(int* num);
void datalink_down_to_physical_forward(Frame frame, int link);
void datalink_up_to_network_ack(Frame frame, int link);
void timeout_link_1(CnetEvent ev, CnetTimerID timer, CnetData data);
void timeout_link_2(CnetEvent ev, CnetTimerID timer, CnetData data);
void timeout_link_3(CnetEvent ev, CnetTimerID timer, CnetData data);
void timeout_link_4(CnetEvent ev, CnetTimerID timer, CnetData data);

static CnetTimerID timers[4][WINDOW_SIZE+1];
static char* nodes[7] = {"Australia", "Fiji","New Zealand","Indonesia","Singapore","Malaysia","Brunei"};
static int num_in_window[4] = {0,0,0,0};
static int num_in_buffer[4] = {0,0,0,0};
static Frame window[4][WINDOW_SIZE + 1];
static int buffer_bounds[4][3] = {{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}};
static Frame buffer[4][WINDOW_SIZE+1];
static int frame_expected[4] = {0,0,0,0};
static int ack_expected[4] = {0,0,0,0};
static int next_frame[4] = {0,0,0,0};

static int routing_table[7][7] =
      {{0,1,2,3,3,3,3},   //AUSTRALIA
       {1,0,1,1,1,1,1},   //FIJI
       {1,1,0,1,1,1,1},   //NEW ZEALAND
       {1,1,1,0,2,3,4},   //INDONESIA
       {1,1,1,1,0,2,1},   //SINGAPORE
       {1,1,1,1,2,0,1},   //MALAYSIA
       {1,1,1,1,1,1,0}};  //BRUNEI
