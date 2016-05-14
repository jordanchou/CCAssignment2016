/*************************************************************************

    Sample header file for template.h 

 **************************************************************************/
#include <string.h>
#include <cnet.h>

/************************************************************
 * Constant data structure. 
 *
 * NOTE: These are just examples 
 *       They are currently not used.
 * 
 ************************************************************/
/**/
#define dt_data      (1)
#define dt_hello     (2)
#define dt_hello_ack (3)

#define R_TABLE_SIZE 30
#define MAX_FRAME_SIZE 1024

/**/

/************************************************************
 * Data structure. 
 *
 * NOTE: the following are just example/suggestion. There 
 *       might be missing or extra data field member. 
 *       They are currently not used.
 * 
 ************************************************************/
/**/
typedef struct {
    char data[80];
} DATA;

typedef struct {
    int  port_num;           
    int	 length;
    DATA data;
} Segment;

typedef struct {
    CnetAddr src_addr;
    CnetAddr dest_addr;
    int      type;           
    DATA     data;
} Packet;

typedef struct {
    int    length;       	
    int    checksum;  	
    Packet datagram;
} Frame;

/**/

/************************************************************
 * Variable. 
 *  Note: global variables are allow for simulation. They are
 *  useful if you want to aviod using pointers parameters for 
 *  functions.
 ************************************************************/
Segment current_seg;
Packet current_pkt;
Frame current_frame;

/************************************************************
 * Function Prototypes. 
 *
 ************************************************************/
void reboot_host();
void reboot_router();
static void prompt(int);

/* Data Link Layer */
void network_downto_datalink( int, char*, int* );
static void frame_arrived( CnetEvent, CnetTimerID, CnetData );

/* Network Layer */
void transport_downto_network( int, char*, int*);
void datalink_upto_network( int*, char*, int* );

/* Transport Layer */
void application_downto_transport( int, char*, int* );
void network_upto_transport( int*, char*, int* );

/* Application Layer */
static void keyboard(CnetEvent, CnetTimerID, CnetData );
void transport_upto_application(int*, char*, int* );



