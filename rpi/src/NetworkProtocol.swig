%include "stdint.i"

%module(directors="1") NetworkProtocol
%{
#include "NetworkProtocol.h"
%}

// Enable directors for the MessageCallback class
%feature("director") NetworkProtocol::MessageCallback;

// The NetworkProtocol class
%feature("flatnested");
%include "NetworkProtocol.h"
%feature("flatnested", "");

// Export also the wiringPi initialization function
extern int wiringPiSetup();
