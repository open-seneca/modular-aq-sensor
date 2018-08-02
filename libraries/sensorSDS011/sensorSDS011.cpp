#include "sensorSDS011.h"



SDS021::SDS021(void) {

}


// --------------------------------------------------------
// SDS021::txrCommand()
// Executes and checks communication with the Nova SDS021 and
// SDS021 (maybe more) as documented in 
// Laser Dust Sensor Control Protocol V1.3
// which can (could?) be found
// here: https://cdn.sparkfun.com/assets/parts/1/2/2/7/5/Laser_Dust_Sensor_Control_Protocol_V1.3.pdf
// PC software ( which must be run in Windows 8 compatible mode on Windows 10 or else it will not find COM ports)
// was found here:
// https://d11.baidupcs.com/file/c6eb943e8a08202bf5cd721f023ef8c3?bkt=p3-0000aa6552059697d82e4a8edeee9d287b81&xcode=c2be2503d99003e9627aa2a060c0e38f5487d123a70d3c03fbe59cdc8f3909f058443de5464f1e91&fid=2959855691-250528-662993853982798&time=1522405446&sign=FDTAXGERLQBHSKa-DCb740ccc5511e5e8fedcff06b081203-a27Ffwr9zL799nznmjWLNxTlUww%3D&to=d11&size=9536562&sta_dx=9536562&sta_cs=969&sta_ft=rar&sta_ct=6&sta_mt=5&fm2=MH%2CYangquan%2CAnywhere%2C%2Cnoord-holland%2Cany&vuk=282335&iv=0&newver=1&newfm=1&secfm=1&flow_ver=3&pkey=0000aa6552059697d82e4a8edeee9d287b81&sl=76480590&expires=8h&rt=sh&r=916169565&mlogid=8976280318308348179&vbdid=1848654031&fin=Laser+PM2.5+Sensor+Software+V1.88.rar&fn=Laser+PM2.5+Sensor+Software+V1.88.rar&rtype=1&dp-logid=8976280318308348179&dp-callid=0.1.1&hps=1&tsl=80&csl=80&csign=ZMLyV6T0L9zkkwFfMOo%2F4sxc4LA%3D&so=0&ut=6&uter=4&serv=0&uc=2759677928&ic=3656030823&ti=cdac69781712398020903087bfa8b4c16944a2effab9d69e305a5e1275657320&by=themis
// 
// --------------------------------------------------------

bool SDS021::txrCommand( uint8_t q[19], uint8_t b[10] ){
uint8_t crc=0,i, loopcount, maxwait=30;
bool status=false;
	 // When no answer is received, usually this is because device was just reporting.
     // This happens when device is in reporting mode, as then the device spits out a
     // a continuous stream of data.	 
	 // In such cases. question has to be asked again to get an answer.
	for(loopcount = 0; loopcount < 10 && status == false; ++loopcount){
    
		if( _debug)Serial.println("Send command       ");
	
		for (uint8_t j = 0; j < 19; j++) {
			sds_data->write( q[ j ] );
			if( _debug)Serial.print(q[j], HEX);
		}
		sds_data->flush();
	
		if( _debug )Serial.println("\nReceive response ");
	
	    delay(300);
		
		for(i=0; !sds_data->available() && i < maxwait ; ++i ){
			delay(20);
		}
		
		if( i >= maxwait ){
				if( _debug)Serial.println("No data available, exiting.");
				i = 0;
				status = false;
				if ( (q[2] == 6 && q[3] == 1 && q[4] == 1) ) { 
					// wake from sleep. Usually, immediately reporting is started without answer.
					break;
				}
		}
	
		for( i=0, status=true, crc=0; sds_data->available() && i < 10; ++i) {
			b[i] = sds_data->read();
			if (i>1 && i < 8)crc += b[i];
			if( _debug){ Serial.print(" "); Serial.print(b[i], HEX); }
			switch(i){
				case 0:
					if ( b[0] != 0xAA ){
						if( _debug){ Serial.print("--i (not a header) received "); Serial.println( b[i], HEX );}
						status = false;
					}
					break;
				case 1:
					if ( q[2] != 4 ){ 
						if ( b[1] != 0xC5 ){
							if( _debug){ Serial.print("--i, --i (no answer to this)received"); Serial.println( b[i], HEX ); }
							status = false;
						}
					}else{
						if ( b[1] != 0xC0 ){
							if( _debug){ Serial.print("--i, --i (no answer to this)received "); Serial.println( b[i], HEX);}
							status = false;
						}
					}
					break;
				case 2:
					if ( q[2] != 4 ){ 
						if ( b[2] != q[2] ){
							if( _debug){ Serial.print("--i, --i,--i (wrong command)received "); Serial.println( b[i], HEX );}
							status = false;
						}
					}
					break;
				case 8:
					if ( crc != b[8] ){
						if( _debug){ Serial.print("Checksum error calculated"); Serial.print( crc, HEX );Serial.print(" <> received "); Serial.println(b[8], HEX );}
						status = false;
					}
					break;
				case 9:
					if( b[9] != 0xAB ){
						status = false;
					}
					break;
				default:
					break;
			}
			yield();
		}
		
		if ( i < 10 ) status = false;
		if( _debug){ Serial.println(""); Serial.print("------ end response, read "); Serial.print(i); Serial.print( " bytes: "); Serial.print( status?"ok":"error" ); Serial.println("------"); }	
	}

	
return( status );

}
// --------------------------------------------------------
// SDS021::workMode() datasheet (1)
// result will contain either SDS021_QUERYMODE or SDS021_REPORTMODE,
// depending on how it was set by this function ( by setting
// argument set to SDS021_SET), or how it was set before if argument
// set was set to SDS021_ASK.
//
// --------------------------------------------------------

bool SDS021::workMode( uint8_t *result, uint8_t mode, uint8_t set ){
	                     // 0    1    2   3    4    5    6    7    8    9   10   11   12   13   14         15          16    17    18
	uint8_t qmode[19] ={ 0xAA,0xB4,0x02,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,_devid_one, _devid_two, 0x00, 0xAB}; 
	uint8_t b[10];
	bool status=false;
	
	*result = 0xFF;
	
	if ( set  == SDS021_SET ) qmode[ 3 ] =  SDS021_SET; 
	if ( mode == SDS021_QUERYMODE ) qmode[ 4 ] =  SDS021_QUERYMODE; 
	

    for( uint8_t j=2; j < 17; ++j ) qmode[17] += qmode[j];

	
    if( (status = txrCommand( qmode, b )) ){ 
		if( _debug){
			char line[80];
			sprintf( line, "Device id %02X %02X : Mode = %s\n", b[6],b[7],b[4] == SDS021_QUERYMODE?"Querymode":"Reportingmode" );
			Serial.print ( line );
		}
	
		if ( set == SDS021_SET && b[4] != mode ){
			status = false;
		}
		if ( status ){
			*result = b[4];
		}
	}
	
	return( status );	
}
// --------------------------------------------------------
// SDS021::queryData() datasheet (2)
// returns ppm10 and ppm2.5 values
// --------------------------------------------------------

bool SDS021::queryData( float *ppm10, float *ppm25 ){
	                     // 0    1    2   3    4    5    6    7    8    9   10   11   12   13   14         15          16    17    18
	uint8_t qdata[19] ={ 0xAA,0xB4,0x04,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,_devid_one, _devid_two, 0x00, 0xAB}; 
	uint8_t b[10];
	bool status=false;
	
    for( uint8_t j=2; j < 17; ++j ) qdata[17] += qdata[j];
	
    if( (status = txrCommand( qdata, b )) ){
		*ppm25 = (float) ( b[2] + (b[3]<<8 ) ) / 10.0;
		*ppm10 = (float) ( b[4] + (b[5]<<8 ) ) / 10.0;
		if( _debug){
			Serial.print("Data : ppm10 ");
			Serial.print( *ppm10, 2);
			Serial.print(" ppm2.5 ");
			Serial.print( *ppm25, 2);
			Serial.print(" status : ");
			Serial.println( status);
		}	
	}
	
	return( status );	
}
// --------------------------------------------------------
// SDS021::setDeviceId() datasheet (3)
// Device id is two bytes, the factory sets each device to a unique id.
// New id will be returned in result. 
// By default any device is addressed by address FF FF. This is the default
// for this library as not many people will connect more than one device
// to the same serial lines. If this default (FF FF) is used, class instance
// device id (_devid_one and _devid_two ) will not be updated with this
// new device address, and FF FF will continued to be used to address the device.
// --------------------------------------------------------

bool SDS021::setDeviceId( uint8_t result[2], uint8_t new_one, uint8_t new_two ){
	                   // 0    1    2   3    4    5    6    7    8    9   10   11   12          13          14           15          16     17    18
	uint8_t qid[19] ={ 0xAA,0xB4,0x05,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, new_one, new_two, _devid_one, _devid_two, 0x00, 0xAB}; 
	uint8_t b[10];
	bool status=false;
	
	result[0] = _devid_one;
	result[1] = _devid_two;
	
    for( uint8_t j=2; j < 17; ++j ) qid[17] += qid[j];
	
    if( (status = txrCommand( qid, b )) ){
		if ( b[6] != new_one || b[7] != new_two ) {
			if( _debug){
				char line[80];
				sprintf( line,"Failed setting devid. Asked %02X %02X, device returned %02X %02X\n", new_one, new_two, b[6], b[7]  );
				Serial.print( line );
			}
		
			status = false;
		}else{
			if ( _devid_one != 0xFF || _devid_two != 0xFF ){
				_devid_one = new_one;
				_devid_two = new_two;	
			}
			
			result[0] = b[6];
			result[1] = b[7];
		}
	}
	
	return( status );	
}

// --------------------------------------------------------
// SDS021::sleepWork() datasheet (4)
// result will contain either SDS021_WORKING or SDS021_SLEEPING,
// depending on how it was set by this function ( by setting
// argument set to SDS021_SET), or how it was set before if argument
// set was set to SDS021_ASK.
// --------------------------------------------------------

bool SDS021::sleepWork( uint8_t *result, uint8_t mode,uint8_t set){
	                     // 0    1    2   3    4    5    6    7    8    9   10   11   12   13   14         15          16    17    18
	uint8_t qmode[19] ={ 0xAA,0xB4,0x06,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,_devid_one, _devid_two, 0x00, 0xAB}; 
	uint8_t b[10];
	bool status=false;
	
	*result = 0xFF;
	
	if ( set  == SDS021_SET ) qmode[ 3 ] =  SDS021_SET; 
	if ( mode == SDS021_WORKING ) qmode[ 4 ] =  SDS021_WORKING; 
	

    for( uint8_t j=2; j < 17; ++j ) qmode[17] += qmode[j];

	
    if( (status = txrCommand( qmode, b )) ){ 
		if( _debug){
			char line[80];
			sprintf(line, "Device id %02X %02X : Mode = %s\n", b[6],b[7],b[4] == SDS021_WORKING?"Working":"Sleeping" );
			Serial.print( line );
		}
		*result = b[4];
		if ( set == SDS021_SET && b[4] != mode ){
			status = false;
		}
	}else{
		if ( set == SDS021_SET && mode == SDS021_WORKING ){
			if( _debug)Serial.println("As usual, no answer received after ordering SDS021 to enter SDS021_WORKING mode" );
		}
	}
	return( status );	
}

// --------------------------------------------------------
// SDS021::workPeriod() datasheet (5)
// result will contain 0 ( for continuous mode) or the Interval
// time in minutes (for interval mode). According to the datasheet
// device will sleep during the interval and become active 30 seconds 
// before the interval is reached. 
// --------------------------------------------------------

bool SDS021::workPeriod( uint8_t *result, uint8_t minutes,uint8_t set ){
	                     // 0    1    2   3    4    5    6    7    8    9   10   11   12   13   14           15          16    17    18
	uint8_t qperiod[19] ={ 0xAA,0xB4,0x08,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,_devid_one, _devid_two, 0x00, 0xAB}; 
	uint8_t b[10];
	bool status=false;
	
	*result = 0xFF;
	
	if ( set  == SDS021_SET ) qperiod[ 3 ] =  SDS021_SET; 
	qperiod[ 4 ] =  minutes; 
	

    for( uint8_t j=2; j < 17; ++j ) qperiod[17] += qperiod[j];

	
    if( (status = txrCommand( qperiod, b )) ){ 
		if( _debug){
			char line[80];
			sprintf(line, "Device id %02X %02X : workPeriod = %s %d %s\n", b[6],b[7],b[4] == 0?"Continuous":"Interval = ",b[4] == 0?0:b[4], b[4] == 0?".":"minutes." );
			Serial.print( line );
			}
	}	
	if ( set == SDS021_SET && b[4] != minutes ){
		status = false;
	}
	if ( status ){
		*result = b[4];
	}
	return( status );	
}

// --------------------------------------------------------
// SDS021:firmwareVersion() datasheet (6)
//
// result[3] contains firmware date as YY, MM, DD if succesful, 
// else FF FF FF 
// --------------------------------------------------------

bool SDS021::firmwareVersion( uint8_t result[3] ){
	                         // 0    1    2    3    4    5    6    7    8    9   10   11   12   13   14         15          16    17    18
	uint8_t qfirmware[19] ={ 0xAA,0xB4,0x07,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,_devid_one, _devid_two, 0x00, 0xAB}; 
	uint8_t b[10];
	bool status = false;

	result[0] = result[1] = result[2] = 0xFF;
	
	for( uint8_t j=2; j < 17; ++j ) qfirmware[17] += qfirmware[j];
	
    if( (status = txrCommand( qfirmware, b )) ){ 
		if ( _debug){
				char line[80];
				sprintf(line, "Device id %02X %02X : firmware version 20%02d-%02d-%02d\n", b[6],b[7], b[3],b[4],b[5]);
				Serial.print( line);	
		}		
		result[0] = b[3]; result[1] = b[4]; result[2] = b[5];	
	}
  return( status );
}

// --------------------------------------------------------
// SDS021::setDebug( bool onoff )
// --------------------------------------------------------

void SDS021::setDebug( bool onoff ){
	_debug = onoff;
}


void SDS021::begin(HardwareSerial* serial) {
	serial->begin(9600);
	sds_data = serial;
}
