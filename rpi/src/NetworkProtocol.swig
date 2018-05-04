%include "stdint.i"

%module NetworkProtocol
%{
#include "NetworkProtocol.h"
%}

// We should not pass string length by hand
%apply (char *STRING, int LENGTH) { (const unsigned char* data, uint32_t len) };

// The NetworkProtocol class
%feature("flatnested");
%include "NetworkProtocol.h"
%feature("flatnested", "");

// Export also the wiringPi initialization function
extern int wiringPiSetup();
