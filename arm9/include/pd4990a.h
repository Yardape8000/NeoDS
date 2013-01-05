/*
 *	Header file for the PD4990A Serial I/O calendar & clock.
 */


struct pd4990a_s
{
  int seconds;
  int minutes;
  int hours;
  int days;
  int month;
  int year;
  int weekday;
};

void pd4990a_init(void);
void pd4990a_addretrace(void);
u16 read_4990_testbit(void);
u16 read_4990_databit(void);
u16 read_4990_output();
void neoWrite4990a16(u32 address, u16 data);
void pd4990a_increment_day(void);
void pd4990a_increment_month(void);
