#include "./ass_template.h"

/************************************************************************ 
    
    Sample template for CC200 assignment

    This is a template cnet source (for CC200 assignment) which demonstrate the 
    TCP/IP layered design approach. This program is modified from the keyboard 
    cnet protocol source file example given by CNET. The program is a simple 
    cnet protocol source file which demonstrates the use of the keyboard event 
    and the Physical Layer in cnet.

    To run this demo, run  cnet TEMPLATE, click on both node icons to open
    their output windows, click the 'Run' button, click in one of the node's
    output windows, type your name and finally press RETURN.

    Note that this simple ``protocol'' will not work if there is corruption
    on the link (probframecorrupt != 0). Can you see why?

    Note that this simple 'protocol' only send message to a fix destination, so you
    need to (MUST) modify the code to enable different destination node.

***********************************************************************************/

static void prompt(int inc)
{
    static int n=0;

    n += inc;
    printf("%s.%d> ", nodeinfo.nodename, n);
}

/***************************************************************************
 * Application Layer 
 *
 * Note: Currently using keyboard for development & testing only, 
 * but later will have to use CNET enable application to generate msg for 
 * simulation 
 *
 ****************************************************************************/

static void keyboard(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    char	line[80];
    int		length;

    length	= sizeof(line);
    CHECK(CNET_read_keyboard(line, &length));

    if(length > 1) {			/* not just a blank line? */
	printf("\tsending %d bytes - \"%s\"\n", length, line);
	/*CHECK(CNET_write_physical(1, line, &length));*/
	application_downto_transport(1, line, &length);
	prompt(1);
    }
    else
	prompt(0);
}

void transport_upto_application(int *link, char* msg, int* length )
{
   /*
    * NOTE: received msg from transport layer...  
    *
    */
    printf("\treceived %d bytes on link %d - \"%s\"\n",*length,*link,msg);
    prompt(0);
}

/*
static void application_ready(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    char msg[MAX_MESSAGE_SIZE];
    int length;
    CnetAddr destaddr;

    length  = sizeof(msg);
    CHECK(CNET_read_application(&destaddr, (char *)&msg, &length));
    CHECK(CNET_disable_application(ALLNODES));

    application_downto_transport(1, msg, &length);
}
*/

/**************************************************************************** 
 * Transport layer 
 * 
 * If your assignment is to develop transport layer protocol, then the codes
 * are inserted in the following functions. You may create other 
 * supporting/utilite function if required.
 * 
 * Note: this layer is to create a Segment, 
 *       Segment <- (transport layer header) + (msg from the application layer)
 *
 *****************************************************************************/

void application_downto_transport(int link, char *msg, int *length)
{
   /*
    * Construct segment
    * NOTE: encapsulate application msg into Segment here... 
    *
    */


    /*
     * Note: Should hand Segment and destination to the network layer
     */
    transport_downto_network(1, msg, length);
}

void network_upto_transport(int *link, char* segment, int* length )
{
    /*
     * NOTE: retrieve msg from Segment here & pass it backup to
     * the application layer.  
     *
     */
    transport_upto_application(link, segment, length );
}

/*****************************************************************************
 * Network layer
 * 
 * If your assignment is to develop routing protocol, then the codes
 * are inserted in the following functions. You may create other 
 * supporting/utilite function if required.
 * 
 * Note: this layer is to create a Packet/Datagram, 
 *       Packet <- (network layer header) + (Segment)
 *
 *****************************************************************************/

void transport_downto_network(int link, char *msg, int *length)
{
   /*
    * Construct packet/datagram 
    *
    * NOTE: encapsulate segment into packet here... 
    *
    */

    /*
     * Note: Should hand Packet and link number to data link level
     */
    network_downto_datalink(1, msg, length);
}


void datalink_upto_network(int *link, char* packet, int* length )
{
   /*
    * NOTE: retrieve Segment from Packet here & pass it backup to 
    * the transport layer.   
    *
    */
    network_upto_transport(link, packet, length );
}

/*****************************************************************************
 * Data Link Layer 
 *
 * If your assignment is to develop data link layer protocol, then the codes
 * are inserted in the following functions. You may create other 
 * supporting/utilite function if required.
 * 
 * Note: this layer is to create a Frame, 
 *       Frame <- (data link layer header) + (Packet)
 *
 *
 *****************************************************************************/

void network_downto_datalink(int link, char *msg, int *length)
{
   /*
    * NOTE: encapsulate packet/datagrame into Frame here...  
    *
    */

   /*
    * Pass Frame to the CNET physical layer to send Frame to the require link.
    */
   CHECK(CNET_write_physical(1, msg, length));
}

static void frame_arrived(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    char frame[256];
    int  link, length;

   /*
    * NOTE: retrieve Frame from the CNET physical layer.
    *
    */
    length = sizeof(frame);
    CHECK(CNET_read_physical(&link, frame, &length));
    
   /*
    * NOTE: retrieve packet from the Frame & pass it backup to the network layer.   
    *
    */
    datalink_upto_network(&link, frame, &length );
}


/*************************************************************************
 *
 *      Initialisation stuffs
 *
 *************************************************************************/

void reboot_host()
{

    /*  Indicate our interest in certain cnet events */
    printf("Initialising current node as a host.\n");
    
    /* When everything is working properly, you can uncomment this to
     * let the application layer generate the messages for you 
     */
    /*CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));*/

    /* right now, read from the keyboard only */
    CHECK(CNET_set_handler( EV_KEYBOARDREADY, keyboard, 0));
    CHECK(CNET_set_handler( EV_PHYSICALREADY, frame_arrived, 0));

    /* CNET_enable_application(ALLNODES);*/
    prompt(1);
}

void reboot_router()
{

    /*  Indicate our interest in certain cnet events */
    printf("Initialising current node as a router.\n");

    /* NOTE: set any events that is required in your assignment. 
     * current for this simple working example, there isn't any routers
     */
}

void reboot_node(CnetEvent ev, CnetTimerID timer, CnetData data)
{

    printf("Hello world, this is node %d\n", nodeinfo.nodenumber);

    /* CODE to define HOST from ROUTER */
    if (nodeinfo.nlinks == 1)
        reboot_host();
    else if (nodeinfo.nlinks > 1)
        reboot_router();
    else
        printf("Is there anybody out there ?\n");
}

