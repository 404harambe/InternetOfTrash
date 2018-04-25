%module NetworkProtocol
%{
#include "NetworkProtocol.h"
%}

// The NetworkProtocol class
%feature("flatnested");
%include "NetworkProtocol.h"
%feature("flatnested", "");

// Export also the wiringPi initialization function
extern int wiringPiSetup();
