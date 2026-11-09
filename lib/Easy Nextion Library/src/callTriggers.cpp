/*!
 * callTriggers.cpp - Easy library for Nextion Displays
 * Copyright (c) 2020 Athanasios Seitanis < seithagta@gmail.com >. 
 * All rights reserved under the library's licence
 */

/*! We made a separate file < callTriggers.cpp > for the function callTriggerFunction(), 
 *  as it would be too long to fit in the EasyNextionLibrary.cpp, because there are
 *  50 predefined cases for the triggers and it is easier for someone to edit them
 */

#ifndef EasyNextionLibrary_h
#include "EasyNextionLibrary.h"
#endif                        
                                
#ifndef trigger_h
#include "trigger.h"
#endif
#include "SafeNextionDispatch.h"

void EasyNex::invokeOptionalTrigger(void (*handler)(), uint8_t id) {
  if(!safe_nextion::invokeIfPresent(handler)) rejectCommand(2, id);
}

void EasyNex::callTriggerFunction(){
  
 uint8_t _tempRead = _serial->read();  // We read the next byte, which, according to our protocol, is the < Trigger ID >
                                // From Nextion we send: < printh 23 02 54 xx >
                                // (where xx is the trigger id in HEX, 01 for 1, 02 for 2, ... 0A for 10 etc)
  switch(_tempRead){
    
    case 0x00:
      invokeOptionalTrigger(trigger0, _tempRead);
      break;    
    
    case 0x01:
      invokeOptionalTrigger(trigger1, _tempRead);
      break;

    case 0x02:
      invokeOptionalTrigger(trigger2, _tempRead);
      break;
           
    case 0x03:
      invokeOptionalTrigger(trigger3, _tempRead);
      break;
                 
    case 0x04:
      invokeOptionalTrigger(trigger4, _tempRead);
      break;    
                 
    case 0x05:
      invokeOptionalTrigger(trigger5, _tempRead);
      break;   
    
    case 0x06:
      invokeOptionalTrigger(trigger6, _tempRead);
      break;

    case 0x07:
      invokeOptionalTrigger(trigger7, _tempRead);
      break;
           
    case 0x08:
      invokeOptionalTrigger(trigger8, _tempRead);
      break;
                 
    case 0x09:
      invokeOptionalTrigger(trigger9, _tempRead);
      break;    
                 
    case 0x0A:
      invokeOptionalTrigger(trigger10, _tempRead);
      break;
    
    case 0x0B:
      invokeOptionalTrigger(trigger11, _tempRead);
      break;

    case 0x0C:
      invokeOptionalTrigger(trigger12, _tempRead);
      break;
           
    case 0x0D:
      invokeOptionalTrigger(trigger13, _tempRead);
      break;
                 
    case 0x0E:
      invokeOptionalTrigger(trigger14, _tempRead);
      break;    
                 
    case 0x0F:
      invokeOptionalTrigger(trigger15, _tempRead);
      break;   
    
    case 0x10:
      invokeOptionalTrigger(trigger16, _tempRead);
      break;

    case 0x11:
      invokeOptionalTrigger(trigger17, _tempRead);
      break;
           
    case 0x12:
      invokeOptionalTrigger(trigger18, _tempRead);
      break;
                 
    case 0x13:
      invokeOptionalTrigger(trigger19, _tempRead);
      break;    
                 
    case 0x14:
      invokeOptionalTrigger(trigger20, _tempRead);
      break; 

    case 0x15:
      invokeOptionalTrigger(trigger21, _tempRead);
      break;

    case 0x16:
      invokeOptionalTrigger(trigger22, _tempRead);
      break;
           
    case 0x17:
      invokeOptionalTrigger(trigger23, _tempRead);
      break;
                 
    case 0x18:
      invokeOptionalTrigger(trigger24, _tempRead);
      break;    
                 
    case 0x19:
      invokeOptionalTrigger(trigger25, _tempRead);
      break;   
    
    case 0x1A:
      invokeOptionalTrigger(trigger26, _tempRead);
      break;

    case 0x1B:
      invokeOptionalTrigger(trigger27, _tempRead);
      break;
           
    case 0x1C:
      invokeOptionalTrigger(trigger28, _tempRead);
      break;
                 
    case 0x1D:
      invokeOptionalTrigger(trigger29, _tempRead);
      break;    
                 
    case 0x1E:
      invokeOptionalTrigger(trigger30, _tempRead);
      break;
    
    case 0x1F:
      invokeOptionalTrigger(trigger31, _tempRead);
      break;

    case 0x20:
      invokeOptionalTrigger(trigger32, _tempRead);
      break;
           
    case 0x21:
      invokeOptionalTrigger(trigger33, _tempRead);
      break;
                 
    case 0x22:
      invokeOptionalTrigger(trigger34, _tempRead);
      break;    
                 
    case 0x23:
      invokeOptionalTrigger(trigger35, _tempRead);
      break;   
    
    case 0x24:
      invokeOptionalTrigger(trigger36, _tempRead);
      break;

    case 0x25:
      invokeOptionalTrigger(trigger37, _tempRead);
      break;
           
    case 0x26:
      invokeOptionalTrigger(trigger38, _tempRead);
      break;
                 
    case 0x27:
      invokeOptionalTrigger(trigger39, _tempRead);
      break;    
                 
    case 0x28:
      invokeOptionalTrigger(trigger40, _tempRead);
      break; 
    
    case 0x29:
      invokeOptionalTrigger(trigger41, _tempRead);
      break;

    case 0x2A:
      invokeOptionalTrigger(trigger42, _tempRead);
      break;
           
    case 0x2B:
      invokeOptionalTrigger(trigger43, _tempRead);
      break;
                 
    case 0x2C:
      invokeOptionalTrigger(trigger44, _tempRead);
      break;    
                 
    case 0x2D:
      invokeOptionalTrigger(trigger45, _tempRead);
      break;   
    
    case 0x2E:
      invokeOptionalTrigger(trigger46, _tempRead);
      break;

    case 0x2F:
      invokeOptionalTrigger(trigger47, _tempRead);
      break;
           
    case 0x30:
      invokeOptionalTrigger(trigger48, _tempRead);
      break;
                 
    case 0x31:
      invokeOptionalTrigger(trigger49, _tempRead);
      break;    
                 
    case 0x32:
      invokeOptionalTrigger(trigger50, _tempRead);
      break;
  }
}
