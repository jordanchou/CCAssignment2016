/*
FILE:assignment.c
AUTHOR:Jordan Pinglin Chou
USERNAME:18348691
UNIT: Computer Communications
PURPOSE: cnet stuff
REFERENCE:-
COMMENTS:-
REQUIRES:-
*/

#include "assignment.h"

void draw_frame(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    CnetDrawFrame *df  = (CnetDrawFrame *)data;
    Frame         *f   = (Frame *)df->frame;

    switch (f->kind) {
    case ACK:
        df->nfields    = 1;
        df->colours[0] = (f->sequence == 0) ? "red" : "purple";
        df->pixels[0]  = 10;
        sprintf(df->text, "ack=%d", f->sequence);
        break;

    case DATA:
        df->nfields    = 2;
        df->colours[0] = (f->sequence == 0) ? "red" : "purple";
        df->pixels[0]  = 10;
        df->colours[1] = "green";
        df->pixels[1]  = 30;
        sprintf(df->text, "data=%d", f->sequence);
        break;
    }
}

/**
*
* @param ev    [description]
* @param timer [description]
* @param data  [description]
*/
void reboot_node(CnetEvent ev, CnetTimerID timer, CnetData data)
{
   //Set handlers for the required events
   CHECK(CNET_set_handler(EV_DRAWFRAME, &draw_frame, 0));
   CHECK(CNET_set_handler(EV_APPLICATIONREADY, application_down_to_network, 0));
   CHECK(CNET_set_handler(EV_PHYSICALREADY, physical_up_to_datalink, 0));
   CHECK(CNET_set_handler(EV_TIMER1, &timeout_link_1, 0));
   CHECK(CNET_set_handler(EV_TIMER2, &timeout_link_2, 0));
   CHECK(CNET_set_handler(EV_TIMER3, &timeout_link_3, 0));
   CHECK(CNET_set_handler(EV_TIMER4, &timeout_link_4, 0));

   //Initialise the timers for each link the node has
   for (int ii=0; ii<nodeinfo.nlinks; ii++)
   {
       for (int jj=0; jj<=WINDOW_SIZE; jj++)
       {
           timers[ii][jj] = NULLTIMER;
       }
   }

   //Enable the node to be able to send messages
   CHECK(CNET_enable_application(ALLNODES));
}

/**
 * APPLICATION -> NETWORK -> DATALINK -> PHYSICAL
 */

/**
 * [application_down_to_network description]
 * @param ev    [description]
 * @param timer [description]
 * @param data  [description]
 */
void application_down_to_network(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    Frame frame;

    frame.length = sizeof(frame.data);//Set the length of the frame

    //Read the data/message from the application layer
    CHECK(CNET_read_application(&(frame.dest), (char *)(&frame.data), &(frame.length)));
    frame.source = nodeinfo.nodenumber;//Set the source of the frame to be current
    frame.kind = DATA;//This is a data frame
    network_down_to_datalink(frame);//Pass the frame onto the network
}

/**
 * [network_down_to_datalink description]
 * @param frame [description]
 */
void network_down_to_datalink(Frame frame)
{
    int link;

    //Obtain the link to send from from the routing table
    link = routing_table[nodeinfo.nodenumber][frame.dest];
    frame.sequence = next_frame[link-1];
    window[link-1][frame.sequence] = frame;

    num_in_window[link-1] = num_in_window[link-1] + 1;

    //Transmit the frame
    datalink_down_to_physical_transmit(frame, link);
    increment(&next_frame[link-1]);//Increment the number of frames

    if (num_in_window[link-1] >= WINDOW_SIZE)//If window is full then stop
    {
        printf("\nDisabling application for link %d\n", link);

        for (int ii=0; ii< 7; ii++)
        {
            if (routing_table[nodeinfo.nodenumber][ii] == link)
            {
                CHECK(CNET_disable_application(ii));
            }
        }
    }
}

/**
 * [datalink_down_to_physical description]
 * @param frame [description]
 * @param link  [description]
 */
void datalink_down_to_physical_transmit(Frame frame, int link)
{
    size_t frame_length;

    //Print out the appropriate message depending on the frame type
    if (frame.kind == DATA)
    {
        printf("\nDATA TRANSMITTED\nSOURCE: %s\nDEST: %s\nTHROUGH LINK: %d\nSEQUENCE: %d\nWINDOW SIZE: %d\n",nodes[frame.source], nodes[frame.dest], link, frame.sequence,num_in_window[link-1] );

        set_timer(frame, link, frame.sequence);
    }
    else if (frame.kind == ACK)
    {
        printf("\nACK TRANSMITTED\nSOURCE: %s\nDEST: %s\nTHROUGH LINK: %d\nSEQUENCE: %d\n", nodes[nodeinfo.nodenumber],nodes[frame.dest], link,  frame.sequence);
    }
    else
    {
        printf("\nwtf");//Shouldn't ever get to here
    }

    //Set the checksum of the frame
    frame.checksum = 0;
    frame_length = FRAME_SIZE(frame);
    frame.checksum = CNET_ccitt((unsigned char*)&frame, frame_length);

    //Present the physical layer with the message
    CHECK(CNET_write_physical(link, (char*)&frame, &frame_length));
}

/**
 * [datalink_down_to_physical_forward description]
 * @param frame [description]
 * @param link  [description]
 */
void datalink_down_to_physical_forward(Frame frame, int link)
{
    int out_link;

    out_link = routing_table[nodeinfo.nodenumber][frame.dest];

    if (num_in_window[out_link-1] < WINDOW_SIZE)
    {
        printf("\n\t\t\t\t\tFORWARDING FRAME\n\t\t\t\t\tVIA LINK: %d\n", out_link);

        //Transmit the acknowledgement for the received frame
        datalink_down_to_physical_ack(frame.source, link, frame.sequence);

        //increment the bottom of the window
        increment(&frame_expected[link - 1]);

        //Set the new sequence number of the frame and add it to window
        frame.sequence = next_frame[out_link - 1];
        window[out_link - 1][next_frame[out_link - 1]] = frame;
        num_in_window[out_link - 1] += 1;

        //Transmit the frame and increment the top of the window
        datalink_down_to_physical_transmit(frame, out_link);
        increment(&next_frame[out_link - 1]);
    }
    else
    {
        // If there is space in the buffer then add the frame to the buffer
        if (num_in_buffer[out_link - 1] < WINDOW_SIZE + 1)
        {
            printf("\t\t\t\t\tADDED TO STORE-FORWARD BUFFER\n");

            // Transmit an acknowledgement for the received frame
            datalink_down_to_physical_ack(frame.source, link, frame.sequence);
            // increment the bottom of the window sincremente the frame has been received
            // correctly
            increment(&frame_expected[link - 1]);

            // Add the frame to the buffer
            buffer[out_link - 1][buffer_bounds[out_link - 1][1]] = frame;
            num_in_buffer[out_link - 1] += 1;
            increment(&buffer_bounds[out_link - 1][1]);
        }
    }

    if (num_in_window[out_link - 1] >= WINDOW_SIZE)
    {

        printf("\nDisabling application for link %d\n", out_link);
        for (int ii=0; ii< 7; ii++)
        {
            if (routing_table[nodeinfo.nodenumber][ii] == out_link)
            {
                CHECK(CNET_disable_application(ii));
            }
        }
    }

}

/**
 * [datalink_down_to_physical_ack description]
 * @param link     [description]
 * @param sequence [description]
 */
void datalink_down_to_physical_ack(int dest, int link, int sequence)
{
    Frame frame;

    //Make an ACK frame
    frame.kind = ACK;
    frame.length = 0;
    frame.sequence = sequence;
    frame.dest =dest;
    datalink_down_to_physical_transmit(frame, link);
}


/**
 * PHYSICAL -> DATALINK -> NETWORK -> APPLICATION
 */

/**
 * [physical_up_to_datalink description]
 * @param ev    [description]
 * @param timer [description]
 * @param data  [description]
 */
void physical_up_to_datalink(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    Frame frame;
    int link;
    size_t length;

    length = sizeof(Frame);
    CHECK(CNET_read_physical(&link, (char*)&frame, &length));

    datalink_up_to_network(frame, link, length);
}

/**
 * [datalink_up_to_network description]
 * @param frame  [description]
 * @param link   [description]
 * @param length [description]
 */
void datalink_up_to_network(Frame frame, int link, int length)
{
    int checksum;

    checksum = frame.checksum;
    frame.checksum = 0;

    if (CNET_ccitt((unsigned char*)&frame, (int)length) != checksum)
    {
        printf("\n\t\t\t\t\tFRAME RECEIVED\n\t\t\t\t\tBAD CHECKSUM\n");
        return;
    }

    if (frame.kind == DATA)
    {

        if (frame.sequence == frame_expected[link-1])
        {
            network_up_to_application(frame, link);
        }
        else if (frame.sequence == (frame_expected[link-1] + WINDOW_SIZE) % (WINDOW_SIZE + 1))
        {
            printf("\t\t\t\t\tFRAME ALREADY RECEIVED: RE-TRANSMIT ACK\n");
            datalink_down_to_physical_ack(frame.source, link, frame.sequence);
        }
        else
        {
            printf("\n\t\t\t\t\tDATA RECEIVED\n"
                           "\t\t\t\t\tSOURCE:  %s\n"
                           "\t\t\t\t\tDEST:    %s\n"
                           "\t\t\t\t\tIN LINK: %d\n"
                           "\t\t\t\t\tSEQ NO:  %d\n",
                           nodes[frame.source], nodes[frame.dest], link, frame.sequence );
            printf("\t\t\t\t\tWRONG SEQUENCE NUMBER, DROPPED.\n\t\t\t\t\tEXPECTING: %d GOT: %d\n", frame_expected[link-1], frame.sequence);
        }
    }
    else if (frame.kind == ACK)
    {
        datalink_up_to_network_ack(frame, link);//datalink_up_to_network_ack
    }
    else
    {
        printf("wtf\n");
    }
}

/**
 * [datalink_up_to_network_ack description]
 * @param frame [description]
 * @param link  [description]
 */
void datalink_up_to_network_ack(Frame frame, int link)
{
    Frame temp_frame;
    int out_link;

    printf("\n\t\t\t\t\tACK RECEIVED\n"
           "\t\t\t\t\tDEST:    %s\n"
           "\t\t\t\t\tIN LINK: %d\n"
           "\t\t\t\t\tSEQ NO:  %d\n"
           "\t\t\t\t\tWINDOW SIZE: %d\n",
           nodes[nodeinfo.nodenumber], link, frame.sequence, num_in_window[link-1] -1);

    if (frame.sequence != ack_expected[link-1])
    {
        printf("\n\t\t\t\t\tRESENDING FRAMES UP TO %d\n", ack_expected[link-1]);
    }

    //An acknowledgement acknowledges the frame with the same sequence number
    //and all those sent before it that haven't been acknowledged.
    while (BETWEEN(ack_expected[link - 1], frame.sequence, next_frame[link - 1]))
    {
        //Stop the timer and remove the acknowledged frame from the window
        CNET_stop_timer(timers[link - 1][ack_expected[link - 1]]);
        increment(&ack_expected[link - 1]); // increment the lower bound of the window
        num_in_window[link - 1] -= 1;

    }

    //While there is space in the window and there are frames in the buffer to
    //be sent send these frames
    while (num_in_window[link - 1] < WINDOW_SIZE && num_in_buffer[link - 1] > 0)
    {
        printf("\n\t\t\t\t\tFORWARDING FROM BUFFER\n");

        // Remove the first frame from the buffer and update the buffer bounds
        temp_frame = buffer[link - 1][buffer_bounds[link - 1][0]];
        increment(&buffer_bounds[link - 1][0]); // increment the start index
        num_in_buffer[link - 1] -= 1;

        // Set the sequence number of the frame and add it to the window
        temp_frame.sequence = next_frame[link - 1];
        window[link - 1][next_frame[link - 1]] = temp_frame;
        num_in_window[link - 1] += 1;

        // Determine which link to send the frame on and then send the frame
        out_link = routing_table[nodeinfo.nodenumber][temp_frame.dest];
        datalink_down_to_physical_transmit(temp_frame, out_link);
        increment(&next_frame[link - 1]);
    }

    // If none of the links connected to this node have full windows then enable
    // the application layer. If this application layer is enable then the buffer
    // for each link is also empty
    if (num_in_window[0] < WINDOW_SIZE && num_in_window[1] < WINDOW_SIZE && num_in_window[2] < WINDOW_SIZE && num_in_window[3] < WINDOW_SIZE)
    {
        CHECK(CNET_enable_application(ALLNODES));
    }

}

/**
 * Takes in a frame, if the frame is intended for the current node then it
 * will write the message to the application layer. If the frame is not intended
 * for the current node then it will forward the frame onto the right dest.
 * @param frame The frame containing the message
 * @param link  The link the frame is intended for
 */
void network_up_to_application(Frame frame, int link)
{
    if (frame.dest == nodeinfo.nodenumber)
    {        printf("\n\t\t\t\t\tDATA RECEIVED\n"
                   "\t\t\t\t\tSOURCE:  %s\n"
                   "\t\t\t\t\tDEST:    %s\n"
                   "\t\t\t\t\tIN LINK: %d\n"
                   "\t\t\t\t\tSEQ NO:  %d\n",
                   nodes[frame.source], nodes[frame.dest], link, frame.sequence);
        printf("\t\t\t\t\tUP TO APPLICATION\n");
        datalink_down_to_physical_ack(frame.source, link, frame.sequence);
        increment(&frame_expected[link - 1]);
        CHECK(CNET_write_application((char *)&frame.data, &frame.length));



    }
    else
    {
        printf("\n\t\t\t\t\tDATA RECEIVED\n"
               "\t\t\t\t\tSOURCE:  %s\n"
               "\t\t\t\t\tDEST:    %s\n"
               "\t\t\t\t\tIN LINK: %d\n"
               "\t\t\t\t\tSEQ NO:  %d\n",
               nodes[frame.source], nodes[frame.dest], link, frame.sequence);

        datalink_down_to_physical_forward(frame, link);
    }
}

/**
 * TIMERS
 */

/**
 * [set_timer description]
 * @param frame    [description]
 * @param link     [description]
 * @param sequence [description]
 */
void set_timer(Frame frame, int link, int sequence)
{
    CnetTimerID timer;
    CnetTime timeout;

    timeout = FRAME_SIZE(frame)*((CnetTime)8000000/linkinfo[link].bandwidth) +
                linkinfo[link].propagationdelay;

    switch (link)
    {
        case 1:
            timer = CNET_start_timer(EV_TIMER1, 5 * timeout, (CnetData)sequence);
            break;
        case 2:
            timer = CNET_start_timer(EV_TIMER2, 5 * timeout, (CnetData)sequence);
            break;
        case 3:
            timer = CNET_start_timer(EV_TIMER3, 5 * timeout, (CnetData)sequence);
            break;
        case 4:
            timer = CNET_start_timer(EV_TIMER4, 5 * timeout, (CnetData)sequence);
            break;
    }

    timers[link-1][sequence] = timer;
}

/**
 * Handles timeouts on link 1 by retransmitting the frame
 * @param ev    [description]
 * @param timer [description]
 * @param data  [description]
 */
void timeout_link_1(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    Frame frame;
    int link, sequence;

    link = 1;
    sequence = (int)data;

    frame = window[link-1][sequence];

    printf("\nTIMEOUT\n"
           "SOURCE:   %s\n"
           "DEST:     %s\n"
           "SEQ NO:   %d\n",
           nodes[frame.source], nodes[frame.dest], link, sequence);

    datalink_down_to_physical_transmit(frame, link);
}

/**
 * Handles timeouts on link 2 by retransmitting the frame
 * @param ev    [description]
 * @param timer [description]
 * @param data  [description]
 */
void timeout_link_2(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    Frame frame;
    int link, sequence;

    link = 2;
    sequence = (int)data;

    frame = window[link-1][sequence];

    printf("\nTIMEOUT\n"
           "SOURCE:   %s\n"
           "DEST:     %s\n"
           "SEQ NO:   %d\n",
           nodes[frame.source], nodes[frame.dest], link, sequence);

    datalink_down_to_physical_transmit(frame, link);
}

/**
 * Handles timeouts on link 3 by retransmitting the frame
 * @param ev    [description]
 * @param timer [description]
 * @param data  [description]
 */
void timeout_link_3(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    Frame frame;
    int link, sequence;

    link = 3;
    sequence = (int)data;

    frame = window[link-1][sequence];

    printf("\nTIMEOUT\n"
           "SOURCE:   %s\n"
           "DEST:     %s\n"
           "SEQ NO:   %d\n",
           nodes[frame.source], nodes[frame.dest], link, sequence);

    datalink_down_to_physical_transmit(frame, link);

}

/**
 * Handles timeouts on link 4 by retransmitting the frame
 * @param ev    [description]
 * @param timer [description]
 * @param data  [description]
 */
void timeout_link_4(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    Frame frame;
    int link, sequence;

    link = 4;
    sequence = (int)data;

    frame = window[link-1][sequence];

    printf("\nTIMEOUT\n"
           "SOURCE:   %s\n"
           "DEST:     %s\n"
           "SEQ NO:   %d\n",
           nodes[frame.source], nodes[frame.dest], link, sequence);

    datalink_down_to_physical_transmit(frame, link);

}

/**
 * Acts like a controller. Maintains the condition that the number stored in
 * the pointer must be smaller than the window size
 * @param num the number to increment
 */
void increment(int* num)
{
    if (*num < WINDOW_SIZE)
    {
        *num = *num + 1;
    }
    else
    {
        *num = 0;
    }
}
