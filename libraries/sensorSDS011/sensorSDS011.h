// SDS021 dust sensor PM2.5 and PM10
// ---------------------------------
//
// By R. Zschiegner (rz@madavi.de)
// April 2016
// Expanded for the SDS021
// by Jose Baars, 2018
//
// Documentation:
//		- The iNovaFitness SDS021 datasheet
//

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif


#define SDS021_REPORTMODE 0
#define SDS021_QUERYMODE  1

#define SDS021_ASK 0
#define SDS021_SET 1

#define SDS021_SLEEPING 0
#define SDS021_WORKING  1



class SDS021 {
	public:
		SDS021(void);
		void begin(HardwareSerial* serial);
		bool workMode( 			uint8_t *result, 	uint8_t mode = SDS021_REPORTMODE,	uint8_t set = SDS021_ASK );
		bool queryData( float *ppm10, 		float *ppm25 );
		bool setDeviceId( 		uint8_t result[2], 	uint8_t new_one, uint8_t new_two );
		bool sleepWork( 		uint8_t *result, 	uint8_t awake = SDS021_WORKING,uint8_t set = SDS021_ASK);
		bool workPeriod(		uint8_t *result, 	uint8_t minutes = 0,uint8_t set = SDS021_ASK);
		bool firmwareVersion( 	uint8_t result[3]);
		void setDebug( 			bool onoff );
	private:
		uint8_t _devid_one, _devid_two;
		bool _debug;
		bool txrCommand( uint8_t q[19], uint8_t b[10]);
		Stream *sds_data;		
};
