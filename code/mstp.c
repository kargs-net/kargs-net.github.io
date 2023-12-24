/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2003 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307
 USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

// This clause describes a Master-Slave/Token-Passing (MS/TP) data link 
// protocol, which provides the same services to the network layer as 
// ISO 8802-2 Logical Link Control. It uses services provided by the 
// EIA-485 physical layer. Relevant clauses of EIA-485 are deemed to be 
// included in this standard by reference. The following hardware is assumed:
// (a)	A UART (Universal Asynchronous Receiver/Transmitter) capable of 
//      transmitting and receiving eight data bits with one stop bit 
//      and no parity.
// (b)	An EIA-485 transceiver whose driver may be disabled. 
// (c)	A timer with a resolution of five milliseconds or less

#include <stddef.h>

#define FALSE 0
#define TRUE 1
typedef unsigned char BOOLEAN;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

// MS/TP Frame Type
// Frame Types 8 through 127 are reserved by ASHRAE.
#define FRAME_TYPE_TOKEN 0
#define FRAME_TYPE_POLL_FOR_MASTER 1
#define FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER 2
#define FRAME_TYPE_TEST_REQUEST 3
#define FRAME_TYPE_TEST_RESPONSE 4
#define FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY 5
#define FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY 6
#define FRAME_TYPE_REPLY_POSTPONED 7
// Frame Types 128 through 255: Proprietary Frames
// These frames are available to vendors as proprietary (non-BACnet) frames. 
// The first two octets of the Data field shall specify the unique vendor 
// identification code, most significant octet first, for the type of 
// vendor-proprietary frame to be conveyed. The length of the data portion 
// of a Proprietary frame shall be in the range of 2 to 501 octets.
#define FRAME_TYPE_PROPRIETARY_MIN 128
#define FRAME_TYPE_PROPRIETARY_MAX 255

// MS/TP Frame Format 
// All frames are of the following format:
// 
// Preamble: two octet preamble: X`55', X`FF'
// Frame Type: one octet
// Destination Address: one octet address
// Source Address: one octet address
// Length: two octets, most significant octet first, of the Data field
// Header CRC: one octet
// Data: (present only if Length is non-zero)
// Data CRC: (present only if Length is non-zero) two octets, 
//           least significant octet first
// (pad): (optional) at most one octet of padding: X'FF'

// Used to accumulate the CRC on the data field of a frame.
static UINT16 DataCRC;

// Used to store the data length of a received frame.
static unsigned DataLength;

// Used to store the destination address of a received frame.
static UINT8 DestinationAddress;

// Used to count the number of received octets or errors. 
// This is used in the detection of link activity.
static unsigned EventCount;

// Used to store the frame type of a received frame.
static UINT8 FrameType;

// The number of frames sent by this node during a single token hold. 
// When this counter reaches the value Nmax_info_frames, the node must 
// pass the token.
static unsigned FrameCount;

// Used to accumulate the CRC on the header of a frame.
static UINT8 HeaderCRC;

// Used as an index by the Receive State Machine, up to a maximum value of 
// InputBufferSize.
static unsigned Index;

// The number of elements in the array InputBuffer[].
#define INPUT_BUFFER_SIZE (501)

// An array of octets, used to store octets as they are received. 
// InputBuffer is indexed from 0 to InputBufferSize-1. 
// The maximum size of a frame is 501 octets. 
// A smaller value for InputBufferSize may be used by some implementations.
static UINT8 InputBuffer[INPUT_BUFFER_SIZE];

// "Next Station," the MAC address of the node to which This Station passes 
// the token. If the Next_Station is unknown, Next_Station shall be equal to
// This_Station.
static UINT8 Next_Station;

// "Poll Station," the MAC address of the node to which This Station last 
// sent a Poll For Master. This is used during token maintenance.
static UINT8 Poll_Station;

// A Boolean flag set to TRUE by the Receive State Machine if an error is 
// detected during the reception of a frame. Set to FALSE by the main 
// state machine.
static BOOLEAN ReceivedInvalidFrame;

// A Boolean flag set to TRUE by the Receive State Machine if a valid frame 
// is received. Set to FALSE by the main state machine.
static BOOLEAN ReceivedValidFrame;

// A counter of transmission retries used for Token and Poll For Master 
// transmission.
static unsigned RetryCount;

// A timer with nominal 5 millisecond resolution used to measure and 
// generate silence on the medium between octets. It is incremented by a 
// timer process and is cleared by the Receive State Machine when activity 
// is detected and by the SendFrame procedure as each octet is transmitted. 
// Since the timer resolution is limited and the timer is not necessarily 
// synchronized to other machine events, a timer value of N will actually 
// denote intervals between N-1 and N
static unsigned SilenceTimer;

// A timer used to measure and generate Reply Postponed frames.  It is 
// incremented by a timer process and is cleared by the Master Node State 
// Machine when a Data Expecting Reply Answer activity is completed.
static unsigned ReplyPostponedTimer;

// A Boolean flag set to TRUE by the master machine if this node is the 
// only known master node.
static BOOLEAN SoleMaster;

// Used to store the Source Address of a received frame.
static UINT8 SourceAddress;

// The number of tokens received by this node. When this counter reaches the 
// value Npoll, the node polls the address range between TS and NS for 
// additional master nodes. TokenCount is set to zero at the end of the 
// polling process.
static unsigned TokenCount;

// "This Station," the MAC address of this node. TS is generally read from a 
// hardware DIP switch, or from nonvolatile memory. Valid values for TS are 
// 0 to 254. The value 255 is used to denote broadcast when used as a 
// destination address but is not allowed as a value for TS.
static UINT8 This_Station;
#define MSTP_BROADCAST_ADDRESS 255

// This parameter represents the value of the Max_Info_Frames property of 
// the node's Device object. The value of Max_Info_Frames specifies the 
// maximum number of information frames the node may send before it must 
// pass the token. Max_Info_Frames may have different values on different 
// nodes. This may be used to allocate more or less of the available link 
// bandwidth to particular nodes. If Max_Info_Frames is not writable in a 
// node, its value shall be 1.
static unsigned Nmax_info_frames = 1;

// This parameter represents the value of the Max_Master property of the 
// node's Device object. The value of Max_Master specifies the highest 
// allowable address for master nodes. The value of Max_Master shall be 
// less than or equal to 127. If Max_Master is not writable in a node, 
// its value shall be 127.
static unsigned Nmax_master = 127;

// The number of tokens received or used before a Poll For Master cycle 
// is executed: 50.
static unsigned Npoll = 50;

// The number of retries on sending Token: 1.
static unsigned Nretry_token = 1;

// The minimum number of DataAvailable or ReceiveError events that must be 
// seen by a receiving node in order to declare the line "active": 4.
static unsigned Nmin_octets = 4;

// The minimum time without a DataAvailable or ReceiveError event within 
// a frame before a receiving node may discard the frame: 60 bit times. 
// (Implementations may use larger values for this timeout, 
// not to exceed 100 milliseconds.)
// At 9600 baud, 60 bit times would be about 6.25 milliseconds
static unsigned Tframe_abort = 1 + ((1000 * 60) / 9600);

// The maximum idle time a sending node may allow to elapse between octets 
// of a frame the node is transmitting: 20 bit times.
static unsigned Tframe_gap = 20;

// The time without a DataAvailable or ReceiveError event before declaration 
// of loss of token: 500 milliseconds.
static unsigned Tno_token = 500;

// The maximum time after the end of the stop bit of the final 
// octet of a transmitted frame before a node must disable its 
// EIA-485 driver: 15 bit times.
static unsigned Tpostdrive = 15;

// The maximum time a node may wait after reception of a frame that expects 
// a reply before sending the first octet of a reply or Reply Postponed 
// frame: 250 milliseconds.
static unsigned Treply_delay = 225;

// The minimum time without a DataAvailable or ReceiveError event 
// that a node must wait for a station to begin replying to a 
// confirmed request: 255 milliseconds. (Implementations may use 
// larger values for this timeout, not to exceed 300 milliseconds.)
static unsigned Treply_timeout = 255;

// Repeater turnoff delay. The duration of a continuous logical one state 
// at the active input port of an MS/TP repeater after which the repeater 
// will enter the IDLE state: 29 bit times < Troff < 40 bit times.
static unsigned Troff = 30;

// The width of the time slot within which a node may generate a token: 
// 10 milliseconds.
static unsigned Tslot = 10;

// The minimum time after the end of the stop bit of the final octet of a 
// received frame before a node may enable its EIA-485 driver: 40 bit times.
// At 9600 baud, 40 bit times would be about 4.166 milliseconds
static unsigned Tturnaround = 1 + ((1000 * 40) / 9600);

// The maximum time a node may wait after reception of the token or 
// a Poll For Master frame before sending the first octet of a frame: 
// 15 milliseconds.
static unsigned Tusage_delay = 15;

// The minimum time without a DataAvailable or ReceiveError event that a 
// node must wait for a remote node to begin using a token or replying to 
// a Poll For Master frame: 20 milliseconds. (Implementations may use 
// larger values for this timeout, not to exceed 100 milliseconds.)
static unsigned Tusage_timeout = 20;

// Accumulate "dataValue" into the CRC in crcValue.
// Return value is updated CRC
//
//  Assumes that "unsigned char" is equivalent to one octet.
//  Assumes that "unsigned int" is 16 bits.
//  The ^ operator means exclusive OR.
// Note: This function is copied directly from the BACnet standard.
UINT8 CalcHeaderCRC(UINT8 dataValue, UINT8 crcValue)
{
  unsigned int crc;
 
  crc = crcValue ^ dataValue;	/* XOR C7..C0 with D7..D0 */

  /* Exclusive OR the terms in the table (top down) */
  crc = crc ^ (crc << 1) ^ (crc << 2) ^ (crc << 3)
            ^ (crc << 4) ^ (crc << 5) ^ (crc << 6) 
            ^ (crc << 7);

  /* Combine bits shifted out left hand end */
  return (crc & 0xfe) ^ ((crc >> 8) & 1);
}

// Accumulate "dataValue" into the CRC in crcValue.
//  Return value is updated CRC
//
//  Assumes that "unsigned char" is equivalent to one octet.
//  Assumes that "unsigned int" is 16 bits.
//  The ^ operator means exclusive OR.
// Note: This function is copied directly from the BACnet standard.
UINT16 CalcDataCRC(UINT8 dataValue, UINT16 crcValue)
{
 unsigned int crcLow;

 crcLow = (crcValue & 0xff) ^ dataValue;	/* XOR C7..C0 with D7..D0 */

 /* Exclusive OR the terms in the table (top down) */
 return (crcValue >>8) ^ (crcLow << 8)  ^ (crcLow <<3)
				^ (crcLow <<12)  ^ (crcLow >> 4)
				^ (crcLow & 0x0f) ^ ((crcLow & 0x0f) << 7);
}

// Millisecond Timer - called every millisecond
void MSTP_Millisecond_Timer(void)
{
  if (SilenceTimer < 255)
    SilenceTimer++;
  if (ReplyPostponedTimer < 255)
    ReplyPostponedTimer++;

  return;
}

// Transmits a Frame on the wire
static void SendFrame(
  UINT8 frame_type, // type of frame to send - see defines
  UINT8 destination, // destination address
  UINT8 source,  // source address
  UINT8 *data, // any data to be sent - may be null
  unsigned data_len) // number of bytes of data (up to 501)
{
  UINT8 HeaderCRC; // used for running CRC calculation
  
  (void)frame_type; // FIXME: temp until we implement this code
  (void)destination; // FIXME: temp until we implement this code
  (void)source; // FIXME: temp until we implement this code
  (void)data; // FIXME: temp until we implement this code
  (void)data_len; // FIXME: temp until we implement this code
  // in order to avoid line contention
  while (SilenceTimer < Tturnaround)
  {
    // wait, yield, or whatever
  }

	// Disable the receiver, and enable the transmit line driver.

  // Transmit the preamble octets X'55', X'FF'. 
  // As each octet is transmitted, set SilenceTimer to zero.

  // HeaderCRC = 0xFF;

  // Transmit the Frame Type, Destination Address, Source Address, 
  // and Data Length octets. Accumulate each octet into HeaderCRC. 
  // As each octet is transmitted, set SilenceTimer to zero.

  // Transmit the ones-complement of HeaderCRC. Set SilenceTimer to zero.

  // If there are data octets, initialize DataCRC to X'FFFF'.

  // Transmit any data octets. Accumulate each octet into DataCRC. 
  // As each octet is transmitted, set SilenceTimer to zero.

  // Transmit the ones-complement of DataCRC, least significant octet first. 
  // As each octet is transmitted, set SilenceTimer to zero.

  // Wait until the final stop bit of the most significant CRC octet 
  // has been transmitted but not more than Tpostdrive.

  // Disable the transmit line driver.

  return;
}

static BOOLEAN ReceiveError; // TRUE when error detected during Rx octet
static BOOLEAN DataAvailable; // There is data in the buffer
static UINT8 DataRegister; // stores the latest data 

// called by timer, interrupt(?) or other thread
void Check_UART_Data(void)
{
  if (ReceiveError == TRUE)
  {
    // wait for state machine to clear this
  }
  // wait for state machine to read from the DataRegister
  else if (DataAvailable == FALSE)
  {
    // check for data

    // if error, 
    // ReceiveError = TRUE;
    // return;

    DataRegister = 0; // FIXME: Get this data from UART or buffer

    // if data is ready, 
    // DataAvailable = TRUE;
    // return;
  }
}

// receive FSM states
typedef enum
{
  MSTP_RECEIVE_STATE_IDLE,
  MSTP_RECEIVE_STATE_PREAMBLE,
  MSTP_RECEIVE_STATE_HEADER,
  MSTP_RECEIVE_STATE_HEADER_CRC,
  MSTP_RECEIVE_STATE_DATA,
  MSTP_RECEIVE_STATE_DATA_CRC,
} MSTP_RECEIVE_STATE;

void Receive_Frame_FSM(void)
{
  static MSTP_RECEIVE_STATE state = MSTP_RECEIVE_STATE_IDLE;

  switch (state)
  {
    // In the IDLE state, the node waits for the beginning of a frame.
    case MSTP_RECEIVE_STATE_IDLE:
      // EatAnError
      if (ReceiveError == TRUE)
      {
        ReceiveError = FALSE;
        SilenceTimer = 0; 
        EventCount++;
        // wait for the start of a frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      else 
      {
        if (DataAvailable == TRUE)
        {
          // Preamble1
          if (DataRegister == 0x55)
          {
            DataAvailable = FALSE;
            SilenceTimer = 0;
            EventCount++;
            // receive the remainder of the frame.
            state = MSTP_RECEIVE_STATE_PREAMBLE; 
          }
          // EatAnOctet
          else
          {
            DataAvailable = FALSE;
            SilenceTimer = 0;
            EventCount++;
            // wait for the start of a frame.
            state = MSTP_RECEIVE_STATE_IDLE; 
          }
        }
      }
      break;
    // In the PREAMBLE state, the node waits for the second octet of the preamble.
    case MSTP_RECEIVE_STATE_PREAMBLE:
      // Timeout
      if (SilenceTimer > Tframe_abort)
      {
        // a correct preamble has not been received
        // wait for the start of a frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
    
      // Error
      if (ReceiveError == TRUE)
      {
        ReceiveError = FALSE;
        SilenceTimer = 0;
        EventCount++;
        // wait for the start of a frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      else
      {
        if (DataAvailable == TRUE)
        {
          // Preamble2
          if (DataRegister ==0xFF)
          {
            DataAvailable = FALSE;
            SilenceTimer = 0; 
            EventCount++;
            Index = 0; 
            HeaderCRC = 0xFF;
            // receive the remainder of the frame.
            state = MSTP_RECEIVE_STATE_HEADER; 
          }
          // RepeatedPreamble1
          else if (DataRegister == 0x55)
          {
            DataAvailable = FALSE;
            SilenceTimer = 0; 
            EventCount++;
            // wait for the second preamble octet.
            state = MSTP_RECEIVE_STATE_PREAMBLE; 
          }
          // NotPreamble
          else
          {
            DataAvailable = FALSE;
            SilenceTimer = 0;
            EventCount++;
            // wait for the start of a frame.
            state = MSTP_RECEIVE_STATE_IDLE; 
          }
        }
      }
      break;
    // In the HEADER state, the node waits for the fixed message header.
    case MSTP_RECEIVE_STATE_HEADER:
      // Timeout
      if (SilenceTimer > Tframe_abort)
      {
        // indicate that an error has occurred during the reception of a frame
        ReceivedInvalidFrame = TRUE;
        // wait for the start of a frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
    
      // Error
      if (ReceiveError == TRUE)
      {
        ReceiveError = FALSE;
        SilenceTimer = 0;
        EventCount++;
        // indicate that an error has occurred during the reception of a frame
        ReceivedInvalidFrame = TRUE;
        // wait for the start of a frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      else if (DataAvailable == TRUE)
      {
        // FrameType
        if (Index == 0)
        {
          SilenceTimer = 0; 
          EventCount++;
          HeaderCRC = CalcHeaderCRC(DataRegister,HeaderCRC);
          FrameType = DataRegister;
          DataAvailable = FALSE;
          Index = 1;
          state = MSTP_RECEIVE_STATE_HEADER;
        }
        // Destination
        else if (Index == 1)
        {
          SilenceTimer = 0;
          EventCount++;
          HeaderCRC = CalcHeaderCRC(DataRegister,HeaderCRC);
          DestinationAddress = DataRegister;
          DataAvailable = FALSE;
          Index = 2;
          state = MSTP_RECEIVE_STATE_HEADER;
        }
        // Source
        else if (Index == 2)
        {
          SilenceTimer = 0;
          EventCount++;
          HeaderCRC = CalcHeaderCRC(DataRegister,HeaderCRC);
          SourceAddress = DataRegister;
          DataAvailable = FALSE;
          Index = 3;
          state = MSTP_RECEIVE_STATE_HEADER;
        }
        // Length1
        else if (Index == 3)
        {
          SilenceTimer = 0;
          EventCount++;
          HeaderCRC = CalcHeaderCRC(DataRegister,HeaderCRC);
          DataLength = DataRegister * 256; 
          DataAvailable = FALSE;
          Index = 4;
          state = MSTP_RECEIVE_STATE_HEADER;
        }
        // Length2
        else if (Index == 4)
        {
          SilenceTimer = 0;
          EventCount++;
          HeaderCRC = CalcHeaderCRC(DataRegister,HeaderCRC);
          DataLength += DataRegister;
          DataAvailable = FALSE;
          Index = 5;
          state = MSTP_RECEIVE_STATE_HEADER;
        }
        // HeaderCRC
        else if (Index == 5)
        {
          SilenceTimer = 0;
          EventCount++;
          HeaderCRC = CalcHeaderCRC(DataRegister,HeaderCRC);
          DataAvailable = FALSE;
          state = MSTP_RECEIVE_STATE_HEADER;
        }
        // not per MS/TP standard, but it is a case not covered
        else
        {
          ReceiveError = FALSE;
          SilenceTimer = 0;
          EventCount++;
          // indicate that an error has occurred during the reception of a frame
          ReceivedInvalidFrame = TRUE;
          // wait for the start of a frame.
          state = MSTP_RECEIVE_STATE_IDLE; 
        }
      }
      break;
    // In the HEADER_CRC state, the node validates the CRC on the fixed 
    // message header.
    case MSTP_RECEIVE_STATE_HEADER_CRC:
      // BadCRC
      if (HeaderCRC != 0x55)
      {
        // indicate that an error has occurred during the reception of a frame
        ReceivedInvalidFrame = TRUE;
        // wait for the start of the next frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      else
      {
        if ((DestinationAddress == This_Station) ||
            (DestinationAddress == MSTP_BROADCAST_ADDRESS))
        {
          // FrameTooLong
          if (DataLength > INPUT_BUFFER_SIZE)
          {
            // indicate that a frame with an illegal or unacceptable data length 
            // has been received
            ReceivedInvalidFrame = TRUE;
            // wait for the start of the next frame.
            state = MSTP_RECEIVE_STATE_IDLE; 
          }
          // NoData
          else if (DataLength == 0)
          {
            // indicate that a frame with no data has been received
            ReceivedValidFrame = TRUE;
            // wait for the start of the next frame.
            state = MSTP_RECEIVE_STATE_IDLE; 
          }
          // Data
          else
          {
            Index = 0;
            DataCRC = 0xFFFF;
            // receive the data portion of the frame.
            state = MSTP_RECEIVE_STATE_DATA;  
          }
        }
        // NotForUs
        else
        {
          // wait for the start of the next frame.
          state = MSTP_RECEIVE_STATE_IDLE; 
        }
      }
      break;
    // In the DATA state, the node waits for the data portion of a frame.
    case MSTP_RECEIVE_STATE_DATA:
      // Timeout
      if (SilenceTimer > Tframe_abort)
      {
        // indicate that an error has occurred during the reception of a frame
        ReceivedInvalidFrame = TRUE;
        // wait for the start of the next frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      // Error
      if (ReceiveError == TRUE)
      {
        ReceiveError = FALSE;
        SilenceTimer = 0;
        // indicate that an error has occurred during the reception of a frame
        ReceivedInvalidFrame = TRUE;
        // wait for the start of the next frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      else if (DataAvailable == TRUE)
      {
        // DataOctet
        if (Index < DataLength)
        {
          SilenceTimer = 0;
          DataCRC = CalcDataCRC(DataRegister,DataCRC);
          InputBuffer[Index] = DataRegister;
          DataAvailable = FALSE;
          Index++;
          state = MSTP_RECEIVE_STATE_DATA;
        }
        // CRC1
        if (Index == DataLength)
        {
          SilenceTimer = 0;
          DataCRC = CalcDataCRC(DataRegister,DataCRC);
          DataAvailable = FALSE;
          Index++; // Index now becomes the number of data octets
          state = MSTP_RECEIVE_STATE_DATA;
        }
        // CRC2
        if (Index == (DataLength + 1))
        {
          SilenceTimer = 0;
          DataCRC = CalcDataCRC(DataRegister,DataCRC);
          DataAvailable = FALSE;
          state = MSTP_RECEIVE_STATE_DATA_CRC;
        }
      }
      break;
    // In the DATA_CRC state, the node validates the CRC of the message data.
    case MSTP_RECEIVE_STATE_DATA_CRC:
      // GoodCRC
      if (DataCRC == 0xF0B8)
      {
        // indicate the complete reception of a valid frame
        ReceivedValidFrame = TRUE;

        // now might be a good time to process the message or
        // copy the data to a buffer so that we can process the message
    
        // wait for the start of the next frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      // BadCRC
      else
      {
        // to indicate that an error has occurred during the reception of a frame
        ReceivedInvalidFrame = TRUE;
        // wait for the start of the next frame.
        state = MSTP_RECEIVE_STATE_IDLE; 
      }
      break;
    default:
      // shouldn't get here - but if we do...
      state = MSTP_RECEIVE_STATE_IDLE; 
      break;
  }
  
  return;
}

// master node FSM states
typedef enum
{
  MSTP_MASTER_STATE_INITIALIZE,
  MSTP_MASTER_STATE_IDLE,
  MSTP_MASTER_STATE_USE_TOKEN,
  MSTP_MASTER_STATE_WAIT_FOR_REPLY,
  MSTP_MASTER_STATE_DONE_WITH_TOKEN,
  MSTP_MASTER_STATE_PASS_TOKEN,
  MSTP_MASTER_STATE_NO_TOKEN,
  MSTP_MASTER_STATE_POLL_FOR_MASTER,
  MSTP_MASTER_STATE_ANSWER_DATA_REQUEST,
} MSTP_MASTER_STATE;

void Master_Node_FSM(void)
{
  // When a master node is powered up or reset, 
  // it shall unconditionally enter the INITIALIZE state.
  static MSTP_MASTER_STATE state = MSTP_MASTER_STATE_INITIALIZE;

  switch (state)
  {
    case MSTP_MASTER_STATE_INITIALIZE:
      // DoneInitializing
      This_Station = 0; // FIXME: the node's station address
      // indicate that the next station is unknown
      Next_Station = This_Station; 
      Poll_Station = This_Station;
      // cause a Poll For Master to be sent when this node first 
      // receives the token
      TokenCount = Npoll;
      SoleMaster = FALSE;
      ReceivedValidFrame = FALSE;
      ReceivedInvalidFrame = FALSE;
      state = MSTP_MASTER_STATE_IDLE; 
      break;
    // In the IDLE state, the node waits for a frame.
    case MSTP_MASTER_STATE_IDLE:
      // LostToken
      if (SilenceTimer >= Tno_token)
      {
        // assume that the token has been lost
        state = MSTP_MASTER_STATE_NO_TOKEN;
      }
      // ReceivedInvalidFrame
      else if (ReceivedInvalidFrame == TRUE)
      {
        // invalid frame was received
        ReceivedInvalidFrame = FALSE;
        // wait for the next frame
        state = MSTP_MASTER_STATE_IDLE; 
      }
      // ReceivedUnwantedFrame
      else if (ReceivedValidFrame == TRUE)
      {
        if ((DestinationAddress != This_Station) ||
            (DestinationAddress != MSTP_BROADCAST_ADDRESS))
        {
          // an unexpected or unwanted frame was received.
          ReceivedValidFrame = FALSE;
          // wait for the next frame
          state = MSTP_MASTER_STATE_IDLE; 
        }
        // DestinationAddress is equal to 255 (broadcast) and 
        // FrameType has a value of Token, BACnet Data Expecting Reply, Test_Request, 
        // or a proprietary type known to this node that expects a reply 
        // (such frames may not be broadcast), or
        else if ((DestinationAddress == MSTP_BROADCAST_ADDRESS) &&
             ((FrameType == FRAME_TYPE_TOKEN) ||
              (FrameType == FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY) ||
              (FrameType == FRAME_TYPE_TEST_REQUEST)))
        {
          // an unexpected or unwanted frame was received.
          ReceivedValidFrame = FALSE;
          // wait for the next frame
          state = MSTP_MASTER_STATE_IDLE; 
        }
        // FrameType has a value that indicates a standard or proprietary type
        // that is not known to this node.
        // FIXME: change this if you add a proprietary type
        else if /*(*/(FrameType >= FRAME_TYPE_PROPRIETARY_MIN) /*&&*/
          /*(FrameType <= FRAME_TYPE_PROPRIETARY_MAX))*/
          /* unnecessary if FrameType is UINT8 with max of 255 */
        {
          // an unexpected or unwanted frame was received.
          ReceivedValidFrame = FALSE;
          // wait for the next frame
          state = MSTP_MASTER_STATE_IDLE; 
        }
        // ReceivedToken
        else if ((DestinationAddress == This_Station) &&
                 (FrameType == FRAME_TYPE_TOKEN))
        {
          ReceivedValidFrame = FALSE;
          FrameCount = 0; 
          SoleMaster = FALSE;
          state = MSTP_MASTER_STATE_USE_TOKEN;
        }  
          // ReceivedPFM
        else if ((DestinationAddress == This_Station) &&
                 (FrameType == FRAME_TYPE_POLL_FOR_MASTER))
        {
          SendFrame(
            FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER,
            SourceAddress,
            This_Station,
            NULL,0);
          ReceivedValidFrame = FALSE;
          // wait for the next frame
          state = MSTP_MASTER_STATE_IDLE; 
        }
        // ReceivedDataNoReply
        // or a proprietary type known to this node that does not expect a reply
        else if (((DestinationAddress == This_Station) || 
                  (DestinationAddress == MSTP_BROADCAST_ADDRESS)) &&
                 ((FrameType == FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY) ||
                //  (FrameType == FRAME_TYPE_PROPRIETARY_0) ||
                  (FrameType == FRAME_TYPE_TEST_RESPONSE)))
        {
          // FIXME: indicate successful reception to the higher layers
          // i.e. Process this frame!
          ReceivedValidFrame = FALSE;
          // wait for the next frame
          state = MSTP_MASTER_STATE_IDLE; 
        }
        // ReceivedDataNeedingReply
        // or a proprietary type known to this node that expects a reply
        else if ((DestinationAddress == This_Station) &&
                 ((FrameType == FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY) ||
                //  (FrameType == FRAME_TYPE_PROPRIETARY) ||
                  (FrameType == FRAME_TYPE_TEST_REQUEST)))
        {
          ReplyPostponedTimer = 0;
          // indicate successful reception to the higher layers 
          // (management entity in the case of Test_Request);
          ReceivedValidFrame = FALSE;
          state = MSTP_MASTER_STATE_ANSWER_DATA_REQUEST;
        }
      }
      break;
    // In the USE_TOKEN state, the node is allowed to send one or 
    // more data frames. These may be BACnet Data frames or 
    // proprietary frames.
    case MSTP_MASTER_STATE_USE_TOKEN:
      // NothingToSend
	    // FIXME: If there is no data frame awaiting transmission,
      {
        FrameCount = Nmax_info_frames;
        state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
      }
      // SendNoWait
      // FIXME: If there is a frame awaiting transmission that 
      // is of type Test_Response, BACnet Data Not Expecting Reply, 
      // or a proprietary type that does not expect a reply,
//      {
//        // transmit the data frame
//        SendFrame(?????????????);
//        FrameCount++;
//        state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
//      }
      // SendAndWait
      // FIXME:	If there is a frame awaiting transmission that is of 
      // type Test_Request, BACnet Data Expecting Reply, or 
      // a proprietary type that expects a reply,
//      {
//        // transmit the data frame
//        SendFrame();
//        FrameCount++;
//        state = MSTP_MASTER_STATE_WAIT_FOR_REPLY;
//      }
    // In the WAIT_FOR_REPLY state, the node waits for 
    // a reply from another node.
    case MSTP_MASTER_STATE_WAIT_FOR_REPLY:
      // ReplyTimeout
      if (SilenceTimer >= Treply_timeout)
      {
        // assume that the request has failed
        FrameCount = Nmax_info_frames;
        state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
        // Any retry of the data frame shall await the next entry
        // to the USE_TOKEN state. (Because of the length of the timeout, 
        // this transition will cause the token to be passed regardless 
        // of the initial value of FrameCount.)
      }
      // InvalidFrame
      else if ((SilenceTimer < Treply_timeout) &&
        (ReceivedInvalidFrame == TRUE))
      {
        // error in frame reception
        ReceivedInvalidFrame = FALSE;
        state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
      }
      // ReceivedReply
      // or a proprietary type that indicates a reply
      else if ((SilenceTimer < Treply_timeout) &&
        (ReceivedValidFrame == TRUE) &&
        (DestinationAddress == This_Station) &&
        ((FrameType == FRAME_TYPE_TEST_RESPONSE) ||
         //(FrameType == FRAME_TYPE_PROPRIETARY_0) ||
         (FrameType == FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY)))
      {
        // FIXME: indicate successful reception to the higher layers
        ReceivedValidFrame = FALSE;
        state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
      }
      // ReceivedPostpone
      else if ((SilenceTimer < Treply_timeout) && 
          (ReceivedValidFrame == TRUE) && 
          (DestinationAddress == This_Station) &&
          (FrameType == FRAME_TYPE_REPLY_POSTPONED))
      {
        // FIXME: then the reply to the message has been postponed until a later time.
        // So, what does this really mean?
        ReceivedValidFrame = FALSE;
        state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
      }
      // ReceivedUnexpectedFrame
      else if ((SilenceTimer < Treply_timeout) &&
          (ReceivedValidFrame == TRUE) &&
          (DestinationAddress != This_Station))
      //the expected reply should not be broadcast) 
      {
        // an unexpected frame was received
        // This may indicate the presence of multiple tokens. 
        ReceivedValidFrame = FALSE;
        // Synchronize with the network.
        // This action drops the token.      
        state = MSTP_MASTER_STATE_IDLE;
      }
      // ReceivedUnexpectedFrame
      else if ((SilenceTimer < Treply_timeout) &&
        (ReceivedValidFrame == TRUE) &&
        ((FrameType == FRAME_TYPE_TEST_RESPONSE) ||
         //(FrameType == FRAME_TYPE_PROPRIETARY_0) ||
         (FrameType == FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY)))
      {
        // An unexpected frame was received.
        // This may indicate the presence of multiple tokens. 
        ReceivedValidFrame = FALSE;
        // Synchronize with the network.
        // This action drops the token.      
        state = MSTP_MASTER_STATE_IDLE;
      }
      break;
    // The DONE_WITH_TOKEN state either sends another data frame, 
    // passes the token, or initiates a Poll For Master cycle.
    case MSTP_MASTER_STATE_DONE_WITH_TOKEN:
      // SendAnotherFrame
      if (FrameCount < Nmax_info_frames)
      {
        // then this node may send another information frame 
        // before passing the token. 
        state = MSTP_MASTER_STATE_USE_TOKEN;
      }
      // SoleMaster
      else if ((FrameCount >= Nmax_info_frames) &&
        (TokenCount < Npoll) &&
        (SoleMaster == TRUE))
      {
        // there are no other known master nodes to 
        // which the token may be sent (true master-slave operation). 
        FrameCount = 0;
        TokenCount++;
        state = MSTP_MASTER_STATE_USE_TOKEN;
      }
      // SendToken
      else if (((FrameCount >= Nmax_info_frames) &&
        (TokenCount < Npoll) &&
        (SoleMaster == FALSE)) ||
        // The comparison of NS and TS+1 eliminates the Poll For Master 
        // if there are no addresses between TS and NS, since there is no 
        // address at which a new master node may be found in that case.
        (Next_Station == (UINT8)((This_Station +1) % (Nmax_master + 1))))
      {
        TokenCount++;
        // transmit a Token frame to NS
        SendFrame(
          FRAME_TYPE_TOKEN,
          Next_Station,
          This_Station,
          NULL,0);
        RetryCount = 0;
        EventCount = 0;
        state = MSTP_MASTER_STATE_PASS_TOKEN;
      }
      // SendMaintenancePFM
      else if ((FrameCount >= Nmax_info_frames) &&
        (TokenCount >= Npoll) &&
        ((UINT8)((Poll_Station + 1) % (Nmax_master + 1)) != Next_Station))
      {
        Poll_Station = (Poll_Station + 1) % (Nmax_master + 1);
        SendFrame(
          FRAME_TYPE_POLL_FOR_MASTER,
          Poll_Station,
          This_Station,
          NULL,0);
        RetryCount = 0;
        state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
      }
      // ResetMaintenancePFM
      else if ((FrameCount >= Nmax_info_frames) &&
        (TokenCount >= Npoll) &&
        ((UINT8)((Poll_Station + 1) % (Nmax_master + 1)) == Next_Station) &&
        (SoleMaster == FALSE))
      {
        Poll_Station = This_Station;
        // transmit a Token frame to NS
        SendFrame(
          FRAME_TYPE_TOKEN,
          Next_Station,
          This_Station,
          NULL,0);
        RetryCount = 0;
        TokenCount = 0;
        EventCount = 0;
        state = MSTP_MASTER_STATE_PASS_TOKEN;
      }
      // SoleMasterRestartMaintenancePFM
      else if ((FrameCount >= Nmax_info_frames) &&
        (TokenCount >= Npoll) &&
        ((UINT8)((Poll_Station + 1) % (Nmax_master + 1)) == Next_Station) &&
        (SoleMaster == TRUE))
      {
        Poll_Station = (Next_Station +1) % (Nmax_master + 1);
        SendFrame(
          FRAME_TYPE_POLL_FOR_MASTER,
          Poll_Station,
          This_Station,
          NULL,0);
        // no known successor node
        Next_Station = This_Station;
        RetryCount = 0;
        TokenCount = 0;
        EventCount = 0;
        // find a new successor to TS
        state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
      }
    // The PASS_TOKEN state listens for a successor to begin using
    // the token that this node has just attempted to pass.
    case MSTP_MASTER_STATE_PASS_TOKEN:
      // SawTokenUser
      if ((SilenceTimer < Tusage_timeout) &&
        (EventCount > Nmin_octets))
      {
        // Assume that a frame has been sent by the new token user. 
        // Enter the IDLE state to process the frame.
        state = MSTP_MASTER_STATE_IDLE;
      }
      // RetrySendToken
      else if ((SilenceTimer >= Tusage_timeout) &&
          (RetryCount < Nretry_token))
      {
        RetryCount++;
        // Transmit a Token frame to NS
        SendFrame(
          FRAME_TYPE_TOKEN,
          Next_Station,
          This_Station,
          NULL,0);
        EventCount = 0;
        // re-enter the current state to listen for NS 
        // to begin using the token.
      }
      // FindNewSuccessor
      else if ((SilenceTimer >= Tusage_timeout) &&
          (RetryCount >= Nretry_token))
      {
        // Assume that NS has failed. 
        Poll_Station = (Next_Station + 1) % (Nmax_master + 1);
        // Transmit a Poll For Master frame to PS.
        SendFrame(
          FRAME_TYPE_POLL_FOR_MASTER,
          Poll_Station,
          This_Station,
          NULL,0);
        // no known successor node
        Next_Station = This_Station;
        RetryCount = 0;
        TokenCount = 0;
        EventCount = 0;
        // find a new successor to TS
        state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
      }
      break;
    // The NO_TOKEN state is entered if SilenceTimer becomes greater 
    // than Tno_token, indicating that there has been no network activity 
    // for that period of time. The timeout is continued to determine 
    // whether or not this node may create a token.
    case MSTP_MASTER_STATE_NO_TOKEN:
      // SawFrame
      if ((SilenceTimer < (Tno_token + (Tslot * This_Station))) &&
            (EventCount > Nmin_octets))
      {
        // Some other node exists at a lower address. 
        // Enter the IDLE state to receive and process the incoming frame.
        state = MSTP_MASTER_STATE_IDLE;
      }
      // GenerateToken
      else if ((SilenceTimer >= (Tno_token + (Tslot * This_Station))) &&
          (SilenceTimer < (Tno_token + (Tslot * (This_Station + 1)))))
      {
        // Assume that this node is the lowest numerical address 
        // on the network and is empowered to create a token. 
        Poll_Station = (This_Station + 1) % (Nmax_master + 1);
        // Transmit a Poll For Master frame to PS.
        SendFrame(
          FRAME_TYPE_POLL_FOR_MASTER,
          Poll_Station,
          This_Station,
          NULL,0);
        // indicate that the next station is unknown
        Next_Station = This_Station;
        RetryCount = 0;
        TokenCount = 0;
        EventCount = 0;
        // enter the POLL_FOR_MASTER state to find a new successor to TS.
        state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
      }
      break;
    // In the POLL_FOR_MASTER state, the node listens for a reply to 
    // a previously sent Poll For Master frame in order to find 
    // a successor node.
    case MSTP_MASTER_STATE_POLL_FOR_MASTER:
      // ReceivedReplyToPFM
      if ((ReceivedValidFrame == TRUE) &&
          (DestinationAddress == This_Station) &&
          (FrameType == FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER))
      {
        SoleMaster = FALSE;
        Next_Station = SourceAddress;
        EventCount = 0;
        // Transmit a Token frame to NS
        SendFrame(
          FRAME_TYPE_TOKEN,
          Next_Station,
          This_Station,
          NULL,0);
        Poll_Station = This_Station;
        TokenCount = 0;
        RetryCount = 0;
        ReceivedValidFrame = FALSE;
        state = MSTP_MASTER_STATE_PASS_TOKEN;
      }
      // ReceivedUnexpectedFrame
      else if ((ReceivedValidFrame == TRUE) &&
          ((DestinationAddress != This_Station) ||
           (FrameType != FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER)))
      {
        // An unexpected frame was received. 
        // This may indicate the presence of multiple tokens. 
        ReceivedValidFrame = FALSE;
        // enter the IDLE state to synchronize with the network. 
        // This action drops the token.
        state = MSTP_MASTER_STATE_IDLE;
      }
      // SoleMaster
      else if ((SoleMaster == TRUE) &&
          ((SilenceTimer >= Tusage_timeout) ||
           (ReceivedInvalidFrame == TRUE)))
      {
        // There was no valid reply to the periodic poll 
        // by the sole known master for other masters. 
        FrameCount = 0;
        ReceivedInvalidFrame = FALSE;
        state = MSTP_MASTER_STATE_USE_TOKEN;
      }
      // DoneWithPFM
      else if ((SoleMaster == FALSE) &&
          (Next_Station != This_Station) &&
          ((SilenceTimer >= Tusage_timeout) ||
           (ReceivedInvalidFrame == TRUE)))
      {
        // There was no valid reply to the maintenance 
        // poll for a master at address PS. 
        EventCount = 0;
        // transmit a Token frame to NS
        SendFrame(
          FRAME_TYPE_TOKEN,
          Next_Station,
          This_Station,
          NULL,0);
        RetryCount = 0;
        ReceivedInvalidFrame = FALSE;
        state = MSTP_MASTER_STATE_PASS_TOKEN;
      }
      // SendNextPFM
      else if ((SoleMaster == FALSE) &&
        (Next_Station == This_Station) && // no known successor node
        ((UINT8)((Poll_Station + 1) % (Nmax_master + 1)) != This_Station) &&
        ((SilenceTimer >= Tusage_timeout) ||
         (ReceivedInvalidFrame == TRUE)))
      {
        Poll_Station =  (Poll_Station + 1) % (Nmax_master + 1);
        // Transmit a Poll For Master frame to PS.
        SendFrame(
          FRAME_TYPE_POLL_FOR_MASTER,
          Poll_Station,
          This_Station,
          NULL,0);
        RetryCount = 0;
        ReceivedInvalidFrame = FALSE;
        // Re-enter the current state.
      }
      // DeclareSoleMaster
      else if ((SoleMaster == FALSE) &&
          (Next_Station == This_Station) && // no known successor node
          ((UINT8)((Poll_Station + 1) % (Nmax_master + 1)) == This_Station) &&
          ((SilenceTimer >= Tusage_timeout) ||
           (ReceivedInvalidFrame == TRUE)))
      {
        // to indicate that this station is the only master
        SoleMaster = TRUE;
        FrameCount = 0;
        ReceivedInvalidFrame = FALSE;
        state = MSTP_MASTER_STATE_USE_TOKEN;
      }
      break;
    // The ANSWER_DATA_REQUEST state is entered when a 
    // BACnet Data Expecting Reply, a Test_Request, or 
    // a proprietary frame that expects a reply is received.
    case MSTP_MASTER_STATE_ANSWER_DATA_REQUEST:
      if (ReplyPostponedTimer <= Treply_delay)
      {
        // Reply
        // If a reply is available from the higher layers 
        // within Treply_delay after the reception of the 
        // final octet of the requesting frame 
        // (the mechanism used to determine this is a local matter),
        // then call SendFrame to transmit the reply frame 
        // and enter the IDLE state to wait for the next frame.

        // Test Request
        // If a receiving node can successfully receive and return 
        // the information field, it shall do so. If it cannot receive
        // and return the entire information field but can detect 
        // the reception of a valid Test_Request frame 
        // (for example, by computing the CRC on octets as 
        // they are received), then the receiving node shall discard 
        // the information field and return a Test_Response containing 
        // no information field. If the receiving node cannot detect 
        // the valid reception of frames with overlength information fields, 
        // then no response shall be returned.
        if (FrameType == FRAME_TYPE_TEST_REQUEST)
        {
          SendFrame(
            FRAME_TYPE_TEST_RESPONSE,
            SourceAddress,
            This_Station,
            InputBuffer,Index);
        }
        state = MSTP_MASTER_STATE_IDLE;
      }

      //
      // DeferredReply
      // If no reply will be available from the higher layers
      // within Treply_delay after the reception of the 
      // final octet of the requesting frame (the mechanism 
      // used to determine this is a local matter),
      // then an immediate reply is not possible. 
      // Any reply shall wait until this node receives the token. 
      // Call SendFrame to transmit a Reply Postponed frame, 
      // and enter the IDLE state.

      else
      {
        SendFrame(
          FRAME_TYPE_REPLY_POSTPONED,
          SourceAddress,
          This_Station,
          NULL,0);
        state = MSTP_MASTER_STATE_IDLE;
      }
      break;
    default:
      state = MSTP_MASTER_STATE_IDLE; 
      break;
  }

  return;
}

#ifdef TEST_MSTP
void main(void)
{
  while (TRUE)
  {
    Master_Node_FSM();
    Receive_Frame_FSM();
    Check_UART_Data();
  }
}
#endif
