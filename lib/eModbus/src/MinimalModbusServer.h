// =================================================================================================
// eModbus: Copyright 2020-2022 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#define MINIMAL 
#include "ModbusTypedefs.h"
#include "ModbusError.h"

namespace Modbus {
// States of the server
enum MMSState : uint8_t { MMS_INIT, MMS_LISTEN, MMS_TALK, MMS_ERROR, MMS_STOPPED };

class MinimalModbusServer {
protected:
  MMSState state;                     // Current server state
  virtual void derive();              // Virtual function to prevent direct instances
public:
  // Helper class for message buffers
  class msgbuf {
  protected:
    uint8_t len;

  public:
    inline msgbuf() { clean(); }
    static const uint8_t BUFSIZE = 256;
    uint8_t data[BUFSIZE];
    inline void reset() { len = 0; }
    inline void clean() { 
      for (auto d : data) { 
        d = 0; 
      }
      reset();
    }
    // Methods to access message data
    FunctionCode getFunctionCode();
    uint8_t getServerID();

  };

  msgbuf Request;                     // Request buffer
  msgbuf Response;                    // Response buffer
};

}
