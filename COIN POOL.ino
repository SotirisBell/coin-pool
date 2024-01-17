#include <EEPROM.h>
#include "Ticktick.h"
#include <ShiftDisplay.h>
#include <SimpleRotary.h>
#define SOL 4
#define CA 2  // coin acceptor
#define BUZ 13
#define SER 5
#define BALLS 9
#define dopen 0
#define dclose 1
const int LATCH_PIN = 15;
const int CLOCK_PIN = 16;
const int DATA_PIN = 17;
const DisplayType DISPLAY_TYPE = COMMON_ANODE; // COMMON_CATHODE or COMMON_ANODE
const int DISPLAY_SIZE = 4; // number of digits on display
ShiftDisplay display(LATCH_PIN, CLOCK_PIN, DATA_PIN, DISPLAY_TYPE, DISPLAY_SIZE,
	STATIC_DRIVE);
// Pin A, Pin B, Button Pin
SimpleRotary rotary(7, 6, 10);
#define switched                            true
#define debounce                             1
void SET_DISPLAY();
char VV[4][15] = { "timer", "minutes/coin", "min to start", "solenoid delay" };
int TIMER = 0, MPC = 2, MTS = 2, SOL_DELAY = 3, VALUE = 0, STATUS = 0, solt = 0, sols = 0, BAL = 0, mpc = 0, mts = 0, sdel = 0,pp=0,ps=0;
void Tseconds();
Ticktick timer_seconds(Tseconds, 1000);
long SECS = 0;
int lastClk = HIGH;
int DD = 0, odd = 0;
int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;
volatile  bool IPS = { !true };
bool GET_COIN() {
	int button_reading;

	static bool     switching_pending = false;
	static long int elapse_timer;
	if (IPS == true) {
		
		button_reading = digitalRead(CA);
		if (button_reading == HIGH) {
					switching_pending = true;
			elapse_timer = millis(); // start elapse timing for debounce checking
		}
		if (switching_pending && button_reading == LOW) {
			
			if (millis() - elapse_timer >= debounce) {
				
				switching_pending = false;             // reset for next button press interrupt cycle
				IPS = !true; // reopen ISR for business now button on/off/debounce cycle complete
				return switched;                       // advise that switch has been pressed
			}
		}
	}
	return !switched; // either no press request or debounce period not elapsed
} 
void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	timer_seconds.start();
	pinMode(BALLS, INPUT_PULLUP);
	pinMode(CA, INPUT_PULLUP);
	pinMode(BUZ, OUTPUT);
	pinMode(SOL, OUTPUT);
	digitalWrite(BUZ, 1);
	pinMode(SER, OUTPUT);
	digitalWrite(SER, 0);
	EEPROM.get(10, MPC);
	EEPROM.get(20, MTS);
	EEPROM.get(30, SOL_DELAY);
	mpc = MPC;
	mts = MTS;
	sdel = SOL_DELAY;
	if (MTS < 0) MTS = 0;
	if (MPC < 0) MPC = 0;
	if (SOL_DELAY < 0) SOL_DELAY = 0;
	attachInterrupt(digitalPinToInterrupt(CA), coin, RISING);
	display.set("   0");
	display.update();
}
void loop() {
	timer_seconds.update();
	ROTA();
	if (GET_COIN() == switched) {
		Serial.print("coin.... ");
		SOUND(100);
		if (MPC > 0) SECS = SECS + (MPC * 60);
		if (STATUS == 0 && MTS > 0 && MTS <= SECS / 60)
		{
			STATUS = 1;
			SECS += 5;
			digitalWrite(SOL, HIGH);
			sols = 1;
		}
	}
	else {
		// do other things....
	}
	//	delay(10);
}
void Tseconds()
{
	if (SECS > 0 && STATUS == 1) SECS -= 1;
	if (sols == 1)solt += 1;
	if (solt >= SOL_DELAY)
	{
		digitalWrite(SOL, LOW);
		sols = 0;
	}
	if (SECS == 0)STATUS = 0;
	if (SECS == 60) {
		SOUND(100);
		SOUND(100);
		SOUND(100);
	}
	if (SECS > 0 && SECS < 11) SOUND(100);
	if (SECS == 0 && BAL == dopen)
	{
		pp++;
		if (pp > 6) {
			pp = 0;
			ps++;
		}
		if (ps < 10) {
			if (pp == 4 || pp == 5 || pp == 6)	digitalWrite(SER, 1); delay(200); digitalWrite(SER, 0);
		}
		else
		{
			digitalWrite(SER, 1); delay(200); digitalWrite(SER, 0);
		}
	
	
	}
	FORMAT_TIME();
	BAL = digitalRead(BALLS);
	SET_DISPLAY();
	//if (BAL == 0 && SECS > 0 && STATUS == 1)  SOUND(200); SOUND(200); SOUND(200);SECS = 0;
	Serial.print("  SECS = "); Serial.print(SECS); Serial.print("  DD= "); Serial.print(DD);  Serial.print("  TIMER = "); Serial.print(TIMER); Serial.print("  MPC = "); Serial.print(MPC);
	Serial.print("  MTS = "); Serial.print(MTS); Serial.print("  SOL_DEL = "); Serial.print(SOL_DELAY);
	Serial.print("  BALLs = "); Serial.print(BAL); Serial.print("  display "); Serial.print(VV[DD]); Serial.print(" = "); Serial.println(VALUE);
}
void FORMAT_TIME()
{
	if (SECS > 59)
	{
		TIMER = SECS / 60;
	}
	else
	{
		TIMER = SECS;
	}
}
void SOUND(int d) {
	digitalWrite(BUZ, 0);
	delay(d);
	digitalWrite(BUZ, 1);
}
void coin()
{
	if (IPS == !true) {
		if (digitalRead(CA) == HIGH) {
			IPS = true;  // keep this ISR 'quiet' until button read fully completed
		}
	}
}
void ROTA()
{
	byte d, b;
	// 0 = not turning, 1 = CW, 2 = CCW
	d = rotary.rotate();
	if (d == 1) {
		Serial.println("CW");
		switch (DD)
		{
		case 0:
			break;
		case 1:
			MPC += 1;
			if (MPC > 60)MPC = 60;
			break;
		case 2:
			MTS += 1;
			if (MTS > 60)MTS = 60;
			break;
		case 3:
			SOL_DELAY += 1;
			if (SOL_DELAY > 10)SOL_DELAY = 10;
			break;
		}
	}
	if (d == 2) {
		Serial.println("CCW");
		switch (DD)
		{
		case 0:
			break;
		case 1:
			MPC -= 1;
			if (MPC < 5)MPC = 5;
			break;
		case 2:
			MTS -= 1;
			if (MTS < 5)MTS = 5;
			break;
		case 3:
			SOL_DELAY -= 1;
			if (SOL_DELAY < 1)SOL_DELAY = 1;
			break;
		}
		if (d > 0) SET_DISPLAY();
	}
	// 0 = not pushed, 1 = pushed
	b = rotary.push();
	if (b == 1) {
		Serial.println("Pushed");
		if (DD == 1)
		{
			if (mpc != MPC)
			{
				mpc = MPC;
				EEPROM.put(10, MPC);
			}
		}
		else if (DD == 2)
		{
			if (mts != MTS)
			{
				mts = MTS;
				EEPROM.put(20, MTS);
			}
		}
		else if (DD == 3)
		{
			if (sdel != SOL_DELAY)
			{
				sdel = SOL_DELAY;
				EEPROM.put(30, SOL_DELAY);
			}
		}
		DD += 1;
		if (DD > 3) DD = 0;
		if (DD == 1) mpc = MPC;
		if (DD == 2)mts = MTS;
		if (DD == 3)sdel = SOL_DELAY;
		SOUND(50);
		SET_DISPLAY();
	}
}
void SET_DISPLAY()
{
	String a, b;
	switch (DD)
	{
	case 0:
		b = String(TIMER);
		//if (TIMER < 10)a = "  0" + b;
		//if (TIMER > 9)a = "  " + b;
		display.set(TIMER, 0, ALIGN_RIGHT);
		display.update();
		return;
		break;
	case 1:
		b = String(MPC);
		if (MPC < 10)a = "C  " + b;
		if (MPC > 9)a = "C " + b;
		break;
	case 2:
		b = String(MTS);
		if (MTS < 10)a = "t  " + b;
		if (MTS > 9)a = "t " + b;
		break;
	case 3:
		b = String(SOL_DELAY);
		if (SOL_DELAY < 10)a = "d  " + b;
		if (SOL_DELAY > 9)a = "d " + b;
		break;
	}
	display.set(a);
	display.update();
	Serial.println("display");
}
