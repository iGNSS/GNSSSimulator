#pragma once

#include "trajectoryReader.h"
#include "CoordinateFrameHandler.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#include "Rinex3ObsData.hpp"
#include "Rinex3NavData.hpp"
#include "RinexSatID.hpp"
#include "GPSEphemerisStore.hpp"
#include "Xvt.hpp"
#include "GPSWeek.hpp"
#include "GPSWeekZcount.hpp"
#include "GPSWeekSecond.hpp"

using namespace gpstk;


typedef struct {
	Xvt xvt;
	double pseudorange;
	GPSEphemeris ephemeris;
} mTrajectoryData;
typedef std::map<SatID, std::map<CivilTime, mTrajectoryData>> gps_eph_map;


class trajectoryContainer {

public:
	trajectoryContainer();
	~trajectoryContainer();

	void addObsData();
	void addNavData(const GPSEphemeris& gpseph);
	/* Add these items to the gps_eph_map container */
	void assembleTrajectories(SatID,CivilTime,Xvt,double);		//Store data in containers

	CivilTime listEpochs();			//Print all stored Epochs
	CivilTime listEpochs(SatID);	//Print all stored epochs for a specified sat
	bool isEpochonDarkSide(CivilTime civiliantime, std::vector<CivilTime>& referenceEpochs);

	void write_to_cout_all();		//Write to trajectory format(One file per sat)
	void write_to_cout_test(SatID,CivilTime);

	SatID getSatIDObject(int, SatID::SatelliteSystem );//Return ith SV as SatID object.
	int getSatIDObject_index();
	CivilTime getCivilTimeObject(int year, int month, int day, int hour, int minute, int second);

private:
	
	GPSEphemerisStore ephemerisStore;
	GPSEphemeris ephemeris;
	RinexSatID sat;
	//trajectoryData_c trajectoryData;
	gps_eph_map trajectoryDataContainer;;
	mTrajectoryData trajectoryData;
	


};

