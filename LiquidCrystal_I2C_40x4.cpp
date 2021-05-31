// LiquidCrystal_I2C_40x4 V2.0

#include "LiquidCrystal_I2C_40x4.h"
#include <inttypes.h>
#include "Wire.h"
#include "Arduino.h"


// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

LiquidCrystal_I2C_40x4::LiquidCrystal_I2C_40x4(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows)
{
  _Addr = lcd_Addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _LCDsel = 1;
}

void LiquidCrystal_I2C_40x4::init(){
	init_priv();
}

void LiquidCrystal_I2C_40x4::init_priv()
{
	Wire.begin();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(_cols, _rows);  
	_LCDsel = 2;
	begin(_cols, _rows);  
	_LCDsel = 1;
}

void LiquidCrystal_I2C_40x4::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delayMicroseconds(50000); 
  
	// Now we pull both RS and R/W low to begin commands
	expanderWrite(0x00);	// reset expander
	delay(1000);

  	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
	
	// we start in 8bit mode, try to set 4 bit mode
	write4bits(0x03);
	delayMicroseconds(4500); // wait min 4.1ms
	
	// second try
	write4bits(0x03);
	delayMicroseconds(4500); // wait min 4.1ms
	
	// third go!
	write4bits(0x03); 
	delayMicroseconds(150);
	
	// finally, set to 4-bit interface
	write4bits(0x02); 


	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);  
	
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	
	// clear it off
	clear();
	
	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);
	
	home();
  
}



/********** high level commands, for the user! */
void LiquidCrystal_I2C_40x4::clear(){
	_LCDsel = 1;
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	delayMicroseconds(3000);  // this command takes a long time!
	_LCDsel = 2;
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	delayMicroseconds(3000);  // this command takes a long time!
}

void LiquidCrystal_I2C_40x4::home(){
	_LCDsel = 1;
	command(LCD_RETURNHOME);  // set cursor position to zero
	delayMicroseconds(3000);  // this command takes a long time!
}

void LiquidCrystal_I2C_40x4::setCursor(uint8_t col, uint8_t row){
	if (row < 2) {
		_LCDsel = 1;
	} else {
		_LCDsel = 2;
		row -= 2;
	}
	
	int row_offsets[] = { 0x00, 0x40 };	//, 0x14, 0x54
	
	if ( row >= _numlines ) {
		
		row = _numlines-1;    // we count rows starting w/0

	}
	
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void LiquidCrystal_I2C_40x4::line_blank(uint8_t line_t) {
	setCursor(0,line_t);
	print(F("                                        "));
}

// Turn the display on/off (quickly)
void LiquidCrystal_I2C_40x4::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C_40x4::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystal_I2C_40x4::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C_40x4::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystal_I2C_40x4::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C_40x4::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystal_I2C_40x4::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal_I2C_40x4::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystal_I2C_40x4::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystal_I2C_40x4::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LiquidCrystal_I2C_40x4::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystal_I2C_40x4::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystal_I2C_40x4::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}



/*********** mid level commands, for sending data/cmds */

inline void LiquidCrystal_I2C_40x4::command(uint8_t value) {
	send(value, 0);
}

inline size_t LiquidCrystal_I2C_40x4::write(uint8_t value) {
	send(value, Rs);
	return 0;
}



/************ low level data pushing commands **********/

// write either command or data
void LiquidCrystal_I2C_40x4::send(uint8_t value, uint8_t mode) {
	uint8_t highnib=value>>4;
	uint8_t lownib=value & 0x0F;
	write4bits((highnib)|mode);
	write4bits((lownib)|mode);
}

void LiquidCrystal_I2C_40x4::write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}

void LiquidCrystal_I2C_40x4::expanderWrite(uint8_t _data){  

	Wire.beginTransmission(_Addr);
	Wire.write((int)(_data));
	Wire.endTransmission(); 
}

void LiquidCrystal_I2C_40x4::pulseEnable(uint8_t _data){
if (_LCDsel == 1){
	expanderWrite(_data | En1);	// En high
	delayMicroseconds(1);		// enable pulse must be >450ns
	
	expanderWrite(_data & ~En1);	// En low
	delayMicroseconds(50);		// commands need > 37us to settle
}
else{
	expanderWrite(_data | En2);	// En high
	delayMicroseconds(1);		// enable pulse must be >450ns
	
	expanderWrite(_data & ~En2);	// En low
	delayMicroseconds(50);		// commands need > 37us to settle
	}
} 

void LiquidCrystal_I2C_40x4::sel_LCD(char sel){
	_LCDsel=sel;
}

// Alias functions

void LiquidCrystal_I2C_40x4::cursor_on(){
	cursor();
}

void LiquidCrystal_I2C_40x4::cursor_off(){
	noCursor();
}

void LiquidCrystal_I2C_40x4::blink_on(){
	blink();
}

void LiquidCrystal_I2C_40x4::blink_off(){
	noBlink();
}

void LiquidCrystal_I2C_40x4::load_custom_character(uint8_t char_num, uint8_t *rows){
		createChar(char_num, rows);
}


void LiquidCrystal_I2C_40x4::printstr(const char c[]){
	//This function is not identical to the function used for "real" I2C displays
	//it's here so the user sketch doesn't have to be changed 
	print(c);
}


// unsupported API functions
void LiquidCrystal_I2C_40x4::off(){}
void LiquidCrystal_I2C_40x4::on(){}
void LiquidCrystal_I2C_40x4::setDelay (int cmdDelay,int charDelay) {}
uint8_t LiquidCrystal_I2C_40x4::status(){return 0;}
uint8_t LiquidCrystal_I2C_40x4::keypad (){return 0;}
uint8_t LiquidCrystal_I2C_40x4::init_bargraph(uint8_t graphtype){return 0;}
void LiquidCrystal_I2C_40x4::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end){}
void LiquidCrystal_I2C_40x4::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end){}
void LiquidCrystal_I2C_40x4::setContrast(uint8_t new_val){}

	
