/*
* Original Code copied, modified and adapted from:
* 'khepera3_test.c', available from src/tests
* in libkorebot v.1.19 by KTEAM:
* (c) 2006-2008 EPFL, Lausanne, Switzerland
* Thomas Lochmatter
* &
* 'Sockets Tutorial' by Robert Ingalls:
* http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
*/
#include <korebot/korebot.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#define PULSE_TO_MM 0.03068
#define MM_S_TO_SPEED 218.72
//robot device handlers
static knet_dev_t * dsPic;
static knet_dev_t * mot1;
static knet_dev_t * mot2;

void error(char *msg) {
	perror(msg);
	exit(1);
}

//initializes then configures the motor control unit.
int initMot(knet_dev_t *hDev) {
	if (hDev) {
		kmot_SetMode( hDev , kMotModeIdle );
		kmot_SetSampleTime( hDev , 1550 );
		kmot_SetMargin( hDev , 6 );
		if (hDev == mot1) {
			kmot_SetOptions( hDev , 0x0 , kMotSWOptWindup | kMotSWOptStopMotorBlk | kMotSWOptDirectionInv);
		}
		else {
			kmot_SetOptions( hDev , 0x0 , kMotSWOptWindup | kMotSWOptStopMotorBlk );
		}
		kmot_ResetError( hDev );
		kmot_SetBlockedTime( hDev , 10 );
		kmot_SetLimits( hDev , kMotRegCurrent , 0 , 500 );
		kmot_SetLimits( hDev , kMotRegPos , -10000 , 10000 );
		/* PID Config  */
		kmot_ConfigurePID( hDev , kMotRegSpeed , 620, 3 , 10 );
		kmot_ConfigurePID( hDev, kMotRegPos, 600, 20, 30);
		kmot_SetSpeedProfile(hDev, 15000, 30);
		return 0;
	}
	else {
		printf("initMot error, handle cannot be null\r\n");
		return -1;
	}
}

// initKH3 initialize connections, handles and motors.
int initKH3( void ) {
	/* This is required */
	kh3_init();
	/* open various socket and store the handle in their respective pointers */
	dsPic = knet_open( "Khepera3:dsPic" , KNET_BUS_I2C , 0 , NULL );
	mot1  = knet_open( "Khepera3:mot1" , KNET_BUS_I2C , 0 , NULL );
	mot2  = knet_open( "Khepera3:mot2" , KNET_BUS_I2C , 0 , NULL );

	if (dsPic != 0 && mot1 != 0 && mot2 != 0) {
		initMot(mot1);
		initMot(mot2);
		return 0;
	}
	return -1;
}

//set Motor Speeds
int setSpeeds(float leftSpeed, float rightSpeed) {
	if (mot1 != 0 && mot2 != 0) {
		leftSpeed = (long)(leftSpeed * MM_S_TO_SPEED);
		rightSpeed = (long)(rightSpeed * MM_S_TO_SPEED);
		kmot_SetPoint( mot1 , kMotRegSpeed , leftSpeed);
		kmot_SetPoint( mot2 , kMotRegSpeed , rightSpeed);
		return 0;
	}
	return -1;
}

//stops the motor in the engine control unit.
int motStop() {
	if (mot1 != 0 && mot2 != 0)	{
		kmot_SetMode( mot1 , kMotModeStopMotor);
		kmot_SetMode( mot2 , kMotModeStopMotor);
		return 0;
	}
	return -1;
}


// Main program.
int main(int argc, char *argv[]) {

	int bindsock, sock, portno, clilen;
	char buffer[256];
	char * tok;
	struct sockaddr_in serv_addr, cli_addr;
	int n = 1;
	int i = 0;
	float speeds[2];
	int end = 0;
	float lpos, rpos;

	if (argc < 2) {
		fprintf(stderr, "No port provided\n");
		exit(1);
	}

	if (!initKH3())	{
		printf("Init OK\n");
		//Socket Config
		bindsock = socket(AF_INET, SOCK_STREAM, 0);
		if (bindsock < 0)	error("ERROR opening socket");
		bzero((char *) &serv_addr, sizeof(serv_addr));
		portno = atoi(argv[1]);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(portno);
		if (bind(bindsock, (struct sockaddr *) &serv_addr,
		         sizeof(serv_addr)) < 0)
			error("ERROR on binding");
		listen(bindsock, 5);
		clilen = sizeof(cli_addr);
		sock = accept(bindsock, (struct sockaddr *) &cli_addr, &clilen);
		if (sock < 0) error("ERROR on accept");
		//End Socket Config

		while (n && !end) {
			bzero(buffer, 256);
			strcpy(buffer, "");
			n = read(sock, buffer, 255);
			if (n < 0) error("ERROR reading from socket");
			tok = strtok (buffer, ";");
			while (tok != NULL && i < 2) {
				speeds[i] = atof(tok);
				tok = strtok (NULL, ";");
				i++;
			}
			i = 0;
			printf("leftSpeed: %f / rightSpeed: %f \n", speeds[0], speeds[1]);
			setSpeeds(speeds[0], speeds[1]);
			lpos = kmot_GetMeasure(mot1, kMotRegPos) * PULSE_TO_MM;
			rpos = kmot_GetMeasure(mot2, kMotRegPos) * PULSE_TO_MM;
			sprintf(buffer, "%f;%f", lpos, rpos);
			n = write(sock, buffer, strlen(buffer));
			if (n < 0) error("ERROR writing to socket");
		}

		printf("Stop motors\n");
		motStop();
	}
	printf("Peace out, hombres!\n");
	return 0;
}
