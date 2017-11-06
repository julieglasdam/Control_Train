struct Packet 
{
  byte preambleSize;
  byte address;
  byte order;
  byte checksum;
  int type;
}  trainPacket, accPacket;

// Pointers to reference the instances of the packets
Packet *pTrainPacket;  
Packet *pAccPacket;  

// States for state machine
enum states 
{
  PREAMBLE,
  ADDRESS,
  ORDER,
  CHECKSUM
} state;


byte inputType          = 0xFF;
byte locoAddress        = 0xFF;
byte locoDirection      = 0xFF;
byte locoSpeed          = 0xFF;
byte bitMask            = 0x80; 

int OUTPIN = 3;
unsigned int accessoryNumber = 0xFFFF;
int readyToSend = 0; // 0 = not ready, 1 = send train packet, 2 = send acc packet
int count = 0;


void setup() {
  Serial.begin(9600);
  pinMode(OUTPIN, OUTPUT);
  timer2_setup();

  // Set the trainPacket to default values
  trainPacket.preambleSize         = 12;
  trainPacket.address              = 0xFF;
  trainPacket.order                = 0x00;
  trainPacket.checksum             = 0x00;
  trainPacket.type                 = 0;

  accPacket.preambleSize           = 12;
  accPacket.address                = 0xFF;
  accPacket.order                  = 0x00;
  accPacket.checksum               = 0x00;
  accPacket.type                   = 1;
  

  // Set pointers for packets
  pTrainPacket = &trainPacket;
  pAccPacket = &accPacket;

  // Initialize state to wait for user input
  state = PREAMBLE; 
}


void loop() {
   // Get the input type, to see if the user wants to address train or rails
  inputType = getUserInput(1, "Skriv 0 hvis du vil sende til Tog, og 1 hvis du vil sende til et Accessory");

  // ======= Train =======
  if (inputType == 0) {
      // Get the address, direction and speed, to create a packet to send. The bytes are set to decimal, not binary
      locoAddress   = getUserInput(127, "Vælg hvilket lokomotiv du vil addressere: (1-127).");
      locoDirection = getUserInput(1,   "Vælg hvilken vej du vil køre: (0-1).");
      locoSpeed     = getUserInput(31,  "Vælg hastighed: (0-15), Stop: (0), Emergency Stop: (1).");

      // Set packet with the user input
      setTrainPacket(locoAddress, locoDirection, locoSpeed);
     
      // Send the packets
      readyToSend = 1;
      
  }
  // ======= Accessory =======
  else {
      accessoryNumber = getUserInput(2047, "Select Accessory Number (0-2047).");
      
      setAccPacket(accessoryNumber);

      readyToSend = 2;
      
  }  
}


// Get input from the serial monitor. Just returns the decimal, not a byte  
unsigned int getUserInput(unsigned int max, const char message[]) {
  unsigned int userInput = 0xFFFF;
  Serial.println(message);

  while (userInput > max) {
    if (Serial.available() > 0) {         // Check if something is typed
      userInput = Serial.parseInt();      // Get the int from input   
    }
  }
  return userInput;
}



// Set packets for train
void setTrainPacket(byte address, byte locoDirection, byte locoSpeed){
  // Combine the data from direction and speed to make the order
  byte order = 0x40;
  order |= (locoDirection ? 0x20 : 0);
  order |= (locoSpeed);

  // Set the packet
  pTrainPacket->address = address;
  pTrainPacket->order = order;    
  pTrainPacket->checksum = packetChecksum(pTrainPacket);
}



void setAccPacket(unsigned int accessoryNumber) {
  unsigned int fullAddress    = 0;         
  byte order                  = 0x80;                    
  byte address                = 0x80;                 
  byte output                 = 0;                      

  fullAddress = (accessoryNumber / 4 + 1);
  address |= (0x3F & fullAddress);
  
/*  order |= ((0x1C0 & (0x1C0 ^ fullAddress)) >> 2);

  order |= (direction ? 1 : 0);
  order |= (state ? 0x8 : 0);

  output = (accessoryNumber % 4);
  output = (output == 0) ? 3 : (output - 1);
  order |= (output << 1);

  */


  // ===== Sæt dem her =======
  pAccPacket->address = address;
  pAccPacket->order = order; // Mangler
  //pAccPacket->checksum = packetChecksum(pAccPacket);
}

// Create checksum from address and order
// 0 hvis de er ens eller forskellige?
byte packetChecksum(struct Packet *pPacket) {
  return (pPacket->address ^ pPacket->order);
}

// Prints the converted int-to-binary, but doesn't return the binary
void printBinary(unsigned int b, byte width) {
  unsigned int max = 2;

  for (byte i = 1; i < width; i++) {
    if (b < max) {
      Serial.print("0");
    }
    max *= 2;
  }
  Serial.print(b, BIN);
}


void sendBit(int number) {
  
}

/* NOTER: __________________
 *  // digitalWrite(OUTPIN, HIGH);
  // digitalWrite(OUTPIN, LOW);
    
  

*/


// Timer runs every 58 microseconds 
ISR(TIMER2_COMPA_vect) {
  
  // ===== Send message to train =====
  if (readyToSend == 1) {
    switch(state) {
       
       case PREAMBLE: 
         for (int i = 0; i < 12; i++) {
            Serial.print("1");               // Send 1
         }
         Serial.print(" 0 ");                // Send 0
         state = ADDRESS; 
       break;



       case ADDRESS:
         for (bitMask; bitMask !=0; bitMask >>= 1) {
           if (pTrainPacket->address & bitMask) {
              Serial.print("1");              // Send 1
           }
           else {
              Serial.print("0");              // Send 0
           }
         }
         Serial.print(" 0 ");                 // Send 0
         bitMask = 0x80;
         state = ORDER;    
       break;



       case ORDER:
        for (bitMask; bitMask !=0; bitMask >>= 1) {
           if (pTrainPacket->order & bitMask) {
              Serial.print("1");              // Send 1
           }
           else {
              Serial.print("0");              // Send 0
           }
         }
         Serial.print(" 0 ");                 // Send 0
         bitMask = 0x80;
         state = CHECKSUM; 
       break;



       case CHECKSUM:
        for (bitMask; bitMask !=0; bitMask >>= 1) {
           if (pTrainPacket->checksum & bitMask) {
              Serial.print("1");              // Send 1
           }
           else {
              Serial.print("0");              // Send 0
           }
         }
         Serial.print(" 1 ");                 // Send 1
         bitMask = 0x80; 
         Serial.println();
         Serial.println();
         state = PREAMBLE;
         // Reset so timer doesn't do anything, before user wants to send another message
         readyToSend = 0; 
       break; 
    }     
  }
  
  // ===== Send message to accessory =====
  else if (readyToSend == 2) {
    switch(state) {
      
       case PREAMBLE:
         for (int i = 0; i < 12; i++) {
            Serial.print("1");               // Send 1
         }
         Serial.print(" 0 ");                // Send 0
         state = ADDRESS; 
       break;


       case ADDRESS:
         for (bitMask; bitMask !=0; bitMask >>= 1) {
           if (pAccPacket->address & bitMask) {
              Serial.print("1");              // Send 1
           }
           else {
              Serial.print("0");              // Send 0
           }
         }
         Serial.print(" 0 ");                 // Send 0
         bitMask = 0x80;
       state = ORDER;  
       break;


       case ORDER:
       Serial.print("xxxxxxxx 0 "); 
       state = CHECKSUM; 
       break;


       case CHECKSUM:
       Serial.print("xxxxxxxx 1 "); 
       Serial.println();
       Serial.println();
       state = PREAMBLE;
       // Reset so timer doesn't do anything, before user wants to send another message
       readyToSend = 0; 
       break; 
    }     
  }

  
}



// Setup for the timer
void timer2_setup() 
{
  // Don't get interrupted while setting up the timer
  noInterrupts();
  
  // Set timer2 interrupt at 58 microseconds interval
  TCCR2A = 0;       // set entire TCCR2A register to 0
  TCCR2B = 0;       // same for TCCR2B
  TCNT2  = 0;       // initialize counter value to 0

  // 58 microseconds interval is 17,241 kHz
  // set compare match register for 17 kHz increments:
  // register = (Arduino clock speed) / ((desired interrupt frequency) * prescaler) - 1
  // register = (16*10^6) / (17.241 * 8) - 1 
  // register = 115
  // (must be <256 because timer2 is only 8 bit (timer1 is 16 bit))
  OCR2A = 115; 
  // = 116 ticks before reset (because 0 counts)
  // = 58 microseconds for each tick at 2 MHz

  // Turn on CTC mode
  TCCR2A |= (1 << WGM21);

  // Set CS21 bit for 8 prescaler (and not CS20 or CS22)
  TCCR2B |= (1 << CS21); // = 2 MHz = 0,5 microseconds/tick
  // (0 << CS20) || (0 << CS21) == 0

  // Enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);

  interrupts(); // Reenable interrupts
}

