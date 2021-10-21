   If the server is willing to accept the blocksize option, it sends an
   Option Acknowledgment (OACK) to the client.  The specified value must
   be less than or equal to the value specified by the client.  The
   client must then either use the size specified in the OACK, or send
   an ERROR packet, with error code 8, to terminate the transfer.

   The rules for determining the final packet are unchanged from [1].
   The reception of a data packet with a data length less than the
   negotiated blocksize is the final packet.  If the blocksize is
   greater than the amount of data to be transfered, the first packet is
   the final packet.  If the amount of data to be transfered is an
   integral multiple of the blocksize, an extra data packet containing
   no data is sent to end the transfer.
   