volatile int rate[10];                    // used to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find the inter beat interval
volatile int P = 512;                     // used to find peak in pulse wave
volatile int T = 512;                     // used to find trough in pulse wave
volatile int thresh = 512;                // used to find instant moment of heart beat
volatile int amp = 100;                   // used to hold amplitude of pulse waveform
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = true;       // used to seed rate array so we startup with reasonable BPM

void interruptSetup() {
    // Initializes Timer1 to throw an interrupt every 2mS.
    // The ATtiny85 datasheet (pages 89-92) really helped me out here - and lots of trial and error: http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf
    TCCR1 = 0;
    TCCR1 |= _BV(CTC1);             //clear timer1 when it matches the value in OCR1C
    TCCR1 |= _BV(CS12) | _BV(CS11) | _BV(CS10);    //set prescaler to divide by 64 - 8MHz/64 = 125kHz frequency for each timer tick
    TIMSK |= _BV(OCIE1A);           //enable interrupt when OCR1A matches the timer value
    /**
     * We can do some simple math here for timer calculations
     * GOAL: We want a 2ms trigger, which in Hertz is 500Hz (1/2ms)
     * What we have: 125khz timed clock (thanks to the 64 prescalar)
     * 125khz/500Hz = 250, so we need to count from 1 to 250 (or 0 to 249) and then execute our interrupt service routine
     */
    OCR1A = 249;                    //set the match value for interrupt - 125kHz/250 = 500Hz = 2ms (don't forget we start at a zero count, not 1)
    OCR1C = 249;                    //and the same match value to clear the timer - otherwise it will continue to count and overflow
    sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED
}

// THIS IS THE TIMER 1 INTERRUPT SERVICE ROUTINE.
// Timer 1 makes sure that we take a reading every 2 miliseconds
ISR(TIMER1_COMPA_vect) {                        // triggered when Timer1 counts to 250
    cli();                                      // disable interrupts while we do this

    Signal = analogRead(PULSE_PIN);             // read the Pulse Sensor
    sampleCounter += 2;                         // keep track of the time in mS with this variable
    int N = sampleCounter - lastBeatTime;       // noise delta

    //  find the peak and trough of the pulse wave
    if (Signal < thresh && N > (IBI / 5) * 3) { // avoid dichrotic noise by waiting 3/5 of last IBI
        if (Signal < T) {                       // T is the trough
            T = Signal;                         // keep track of lowest point in pulse wave
        }
    }

    if (Signal > thresh && Signal > P) {        // thresh condition helps avoid noise
        P = Signal;                             // P is the peak
    }                                           // keep track of highest point in pulse wave

    //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
    // signal surges up in value every time there is a pulse
    if (N > 250) {                                  // avoid high frequency noise
        if ( (Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3) ) {
            Pulse = true;                               // set the Pulse flag when we think there is a pulse
            digitalWrite(BLINK_PIN, HIGH);               // turn on pin 13 LED
            IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
            lastBeatTime = sampleCounter;               // keep track of time for next pulse

            if (secondBeat) {                      // if this is the second beat, if secondBeat == TRUE
                secondBeat = false;                  // clear secondBeat flag
                for (int i = 0; i <= 9; i++) {       // seed the running total to get a realisitic BPM at startup
                    rate[i] = IBI;
                }
            }

            if (firstBeat) {                       // if it's the first time we found a beat, if firstBeat == TRUE
                firstBeat = false;                   // clear firstBeat flag
                secondBeat = true;                   // set the second beat flag
                sei();                               // enable interrupts again
                return;                              // IBI value is unreliable so discard it
            }


            // keep a running total of the last 10 IBI values
            word runningTotal = 0;                  // clear the runningTotal variable

            for (int i = 0; i <= 8; i++) {          // shift data in the rate array
                rate[i] = rate[i + 1];                // and drop the oldest IBI value
                runningTotal += rate[i];              // add up the 9 oldest IBI values
            }

            rate[9] = IBI;                          // add the latest IBI to the rate array
            runningTotal += rate[9];                // add the latest IBI to runningTotal
            runningTotal /= 10;                     // average the last 10 IBI values
            BPM = 60000 / runningTotal;             // how many beats can fit into a minute? that's BPM!
            QS = true;                              // set Quantified Self flag
            // QS FLAG IS NOT CLEARED INSIDE THIS ISR
        }
    }

    if (Signal < thresh && Pulse == true) {  // when the values are going down, the beat is over
        digitalWrite(BLINK_PIN, LOW);           // turn off pin 13 LED
        Pulse = false;                         // reset the Pulse flag so we can do it again
        amp = P - T;                           // get amplitude of the pulse wave
        thresh = amp / 2 + T;                  // set thresh at 50% of the amplitude
        P = thresh;                            // reset these for next time
        T = thresh;
    }

    if (N > 2500) {                          // if 2.5 seconds go by without a beat
        thresh = 512;                          // set thresh default
        P = 512;                               // set P default
        T = 512;                               // set T default
        lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date
        firstBeat = true;                      // set these to avoid noise
        secondBeat = false;                    // when we get the heartbeat back
    }

    sei();                                      // enable interrupts when youre done!
}// end isr
